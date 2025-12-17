#ifndef UDP_STREAMER_H
#define UDP_STREAMER_H

#include <string>
#include <vector>
#include <mutex>
#include <set>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../encoding/encoder.h"

// Packet header for fragmentation support
// Added this after realizing keyframes can be > 10KB and exceed MTU
struct PacketHeader {
    uint32_t frameId;           // PTS of the frame
    uint16_t fragmentIndex;     // Which fragment (0-based)
    uint16_t totalFragments;    // Total fragments for this frame
    uint32_t fragmentSize;      // Size of data in this fragment
    uint8_t isKeyframe;         // 1 if keyframe, 0 otherwise
} __attribute__((packed));

// Simple structure to track client connections
struct ClientInfo {
    sockaddr_in address;
    int64_t lastSeenTime;  // Microseconds since epoch

    bool operator<(const ClientInfo& other) const {
        // Compare by address for set ordering
        if (address.sin_addr.s_addr != other.address.sin_addr.s_addr) {
            return address.sin_addr.s_addr < other.address.sin_addr.s_addr;
        }
        return address.sin_port < other.address.sin_port;
    }
};

// UDP streaming server - broadcasts to multiple clients
class UdpStreamer {
public:
    UdpStreamer(int port);
    ~UdpStreamer();

    // Initialize UDP socket
    bool initialize();

    // Get socket FD for control thread
    int getSocketFd() const { return sockfd_; }

    // Send encoded packet to all connected clients
    void sendPacket(const EncodedPacket* packet);

    // Register a new client (called when we receive data from them)
    void registerClient(const sockaddr_in& clientAddr);

    // Clean up old/inactive clients
    void cleanupInactiveClients(int timeoutSeconds = 10);

    // Get statistics
    int getActiveClients() const;
    int64_t getPacketsSent() const { return packetsSent_; }
    int64_t getBytesSent() const { return bytesSent_; }

    void cleanup();

private:
    int port_;
    int sockfd_;
    bool initialized_;

    // Client management
    std::set<ClientInfo> clients_;
    mutable std::mutex clientsMutex_;

    // Statistics
    int64_t packetsSent_;
    int64_t bytesSent_;

    // Helper to split large packets if needed
    void sendPacketFragmented(const uint8_t* data, size_t size, const sockaddr_in& clientAddr);
};

#endif // UDP_STREAMER_H
