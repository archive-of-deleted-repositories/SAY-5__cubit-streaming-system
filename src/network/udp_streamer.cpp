#include "udp_streamer.h"
#include "../utils/config.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <chrono>
#include <errno.h>

UdpStreamer::UdpStreamer(int port)
    : port_(port)
    , sockfd_(-1)
    , initialized_(false)
    , packetsSent_(0)
    , bytesSent_(0) {
}

UdpStreamer::~UdpStreamer() {
    cleanup();
}

bool UdpStreamer::initialize() {
    std::cout << "[Network] Initializing UDP streamer on port " << port_ << std::endl;

    // Create UDP socket
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        std::cerr << "[Network] Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }

    // Set socket options for better performance
    int optval = 1;
    if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        std::cerr << "[Network] Warning: Could not set SO_REUSEADDR" << std::endl;
    }

    // Increase send buffer size for better throughput
    // Started with default but had packet drops at high bitrates
    int sendbuf = 1024 * 1024;  // 1 MB send buffer
    if (setsockopt(sockfd_, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof(sendbuf)) < 0) {
        std::cerr << "[Network] Warning: Could not set send buffer size" << std::endl;
    }

    // Bind to port
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(sockfd_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "[Network] Failed to bind to port " << port_
                  << ": " << strerror(errno) << std::endl;
        close(sockfd_);
        sockfd_ = -1;
        return false;
    }

    initialized_ = true;
    std::cout << "[Network] UDP streamer initialized successfully" << std::endl;
    return true;
}

void UdpStreamer::registerClient(const sockaddr_in& clientAddr) {
    std::lock_guard<std::mutex> lock(clientsMutex_);

    auto now = std::chrono::high_resolution_clock::now();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()).count();

    ClientInfo client;
    client.address = clientAddr;
    client.lastSeenTime = micros;

    auto it = clients_.find(client);
    if (it == clients_.end()) {
        clients_.insert(client);
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, INET_ADDRSTRLEN);
        std::cout << "[Network] New client registered: " << ipStr
                  << ":" << ntohs(clientAddr.sin_port)
                  << " (Total clients: " << clients_.size() << ")" << std::endl;
    } else {
        // Update last seen time
        clients_.erase(it);
        clients_.insert(client);
    }
}

void UdpStreamer::cleanupInactiveClients(int timeoutSeconds) {
    std::lock_guard<std::mutex> lock(clientsMutex_);

    auto now = std::chrono::high_resolution_clock::now();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()).count();

    int64_t timeoutMicros = timeoutSeconds * 1000000LL;

    auto it = clients_.begin();
    while (it != clients_.end()) {
        if (micros - it->lastSeenTime > timeoutMicros) {
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &it->address.sin_addr, ipStr, INET_ADDRSTRLEN);
            std::cout << "[Network] Removing inactive client: " << ipStr << std::endl;
            it = clients_.erase(it);
        } else {
            ++it;
        }
    }
}

void UdpStreamer::sendPacket(const EncodedPacket* packet) {
    if (!initialized_ || !packet || packet->size == 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(clientsMutex_);

    // Fixed: Implement fragmentation for packets larger than MTU
    // Keyframes can be > 10KB which definitely won't fit in one UDP packet
    const size_t maxPayload = Config::MAX_PACKET_SIZE - sizeof(PacketHeader);

    // Calculate number of fragments needed
    uint16_t totalFragments = (packet->size + maxPayload - 1) / maxPayload;

    // Broadcast to all active clients
    for (const auto& client : clients_) {
        for (uint16_t i = 0; i < totalFragments; i++) {
            PacketHeader header;
            header.frameId = static_cast<uint32_t>(packet->pts);
            header.fragmentIndex = i;
            header.totalFragments = totalFragments;
            header.isKeyframe = packet->isKeyframe ? 1 : 0;

            // Calculate fragment size
            size_t offset = i * maxPayload;
            size_t fragmentSize = std::min(maxPayload, packet->size - offset);
            header.fragmentSize = static_cast<uint32_t>(fragmentSize);

            // Build send buffer: header + fragment data
            uint8_t sendBuffer[Config::MAX_PACKET_SIZE];
            memcpy(sendBuffer, &header, sizeof(header));
            memcpy(sendBuffer + sizeof(header), packet->data + offset, fragmentSize);

            size_t totalSize = sizeof(header) + fragmentSize;

            ssize_t sent = sendto(sockfd_, sendBuffer, totalSize, 0,
                                 (struct sockaddr*)&client.address,
                                 sizeof(client.address));

            if (sent < 0) {
                // Don't spam errors - UDP is unreliable by design
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    std::cerr << "[Network] Send error: " << strerror(errno) << std::endl;
                }
            } else {
                bytesSent_ += sent;
            }
        }
    }

    packetsSent_++;

    // Log stats periodically
    if (packetsSent_ % 300 == 0) {  // Every ~5 seconds at 60fps
        double mbSent = bytesSent_ / (1024.0 * 1024.0);
        std::cout << "[Network] Sent " << packetsSent_ << " packets, "
                  << mbSent << " MB to " << clients_.size() << " clients" << std::endl;
    }
}

int UdpStreamer::getActiveClients() const {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    return clients_.size();
}

void UdpStreamer::cleanup() {
    if (!initialized_) {
        return;
    }

    std::cout << "[Network] Cleaning up UDP streamer..." << std::endl;

    if (sockfd_ >= 0) {
        close(sockfd_);
        sockfd_ = -1;
    }

    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        clients_.clear();
    }

    initialized_ = false;
    std::cout << "[Network] Cleanup complete. Sent " << packetsSent_
              << " packets (" << bytesSent_ / (1024.0 * 1024.0) << " MB)" << std::endl;
}
