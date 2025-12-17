#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>
#include <atomic>

#include "utils/config.h"
#include "utils/config_loader.h"
#include "utils/frame.h"
#include "utils/thread_safe_queue.h"
#include "camera/capture.h"
#include "encoding/encoder.h"
#include "network/udp_streamer.h"
#include "monitoring/gpu_monitor.h"
#include "monitoring/performance_tracker.h"
#include "monitoring/adaptation_controller.h"

// Global flag for graceful shutdown
std::atomic<bool> g_running(true);

void signalHandler(int signal) {
    std::cout << "\n[Main] Received signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

// Capture thread - grabs frames from camera
void captureThread(CameraCapture* camera,
                   ThreadSafeQueue<Frame*>* frameQueue,
                   PerformanceTracker* perfTracker) {
    std::cout << "[Capture Thread] Started" << std::endl;

    while (g_running) {
        Frame* cameraFrame = camera->captureFrame();

        if (cameraFrame) {
            // CRITICAL: Deep copy the frame!
            // The camera reuses its buffer, so we need our own copy
            // This was a painful bug to track down - see capture.cpp for details
            Frame* ownedFrame = new Frame(*cameraFrame);

            // Try to push to queue (non-blocking to avoid stalling camera)
            if (!frameQueue->tryPush(ownedFrame)) {
                // Queue full - drop frame
                delete ownedFrame;
                perfTracker->recordFrameDropped();
                // Don't spam logs, but occasionally report drops
                static int dropCount = 0;
                if (++dropCount % 60 == 0) {
                    std::cout << "[Capture Thread] Warning: Dropped " << dropCount
                              << " frames (encoder can't keep up)" << std::endl;
                }
            } else {
                perfTracker->recordFrameProcessed();
            }
        }
    }

    std::cout << "[Capture Thread] Stopped" << std::endl;
}

// Encoding thread - encodes frames using NVENC or CPU
void encodingThread(ThreadSafeQueue<Frame*>* frameQueue,
                   ThreadSafeQueue<EncodedPacket*>* packetQueue,
                   VideoEncoder* encoder,
                   PerformanceTracker* perfTracker) {
    std::cout << "[Encoding Thread] Started" << std::endl;

    while (g_running || !frameQueue->empty()) {
        auto frameOpt = frameQueue->pop();

        if (!frameOpt.has_value()) {
            break;  // Queue stopped
        }

        Frame* frame = frameOpt.value();

        auto encodeStart = std::chrono::high_resolution_clock::now();

        // Encode the frame
        std::vector<EncodedPacket*> packets;
        int numPackets = encoder->encodeFrame(frame, packets);

        auto encodeEnd = std::chrono::high_resolution_clock::now();
        auto encodeTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            encodeEnd - encodeStart).count();

        perfTracker->recordEncodeTime(static_cast<float>(encodeTime));

        // Push encoded packets to network queue
        for (auto* packet : packets) {
            packetQueue->push(packet);
        }

        // Cleanup frame
        delete frame;
    }

    std::cout << "[Encoding Thread] Stopped" << std::endl;
}

// Network thread - sends packets via UDP
void networkThread(ThreadSafeQueue<EncodedPacket*>* packetQueue,
                  UdpStreamer* streamer) {
    std::cout << "[Network Thread] Started" << std::endl;

    while (g_running || !packetQueue->empty()) {
        auto packetOpt = packetQueue->pop();

        if (!packetOpt.has_value()) {
            break;  // Queue stopped
        }

        EncodedPacket* packet = packetOpt.value();

        // Send to all connected clients
        streamer->sendPacket(packet);

        // Cleanup packet
        delete packet;
    }

    std::cout << "[Network Thread] Stopped" << std::endl;
}

// Adaptation thread - monitors performance and adjusts bitrate
void adaptationThread(AdaptationController* controller) {
    std::cout << "[Adaptation Thread] Started" << std::endl;

    while (g_running) {
        controller->update();

        // Check every 2 seconds
        std::this_thread::sleep_for(
            std::chrono::milliseconds(Config::ADAPTATION_INTERVAL_MS));
    }

    std::cout << "[Adaptation Thread] Stopped" << std::endl;
}

// Control thread - listens for client registration packets
// Fixed: This was the missing piece - clients had no way to register!
void controlThread(UdpStreamer* streamer) {
    std::cout << "[Control Thread] Started, waiting for client HELLO packets..." << std::endl;

    int sockfd = streamer->getSocketFd();
    if (sockfd < 0) {
        std::cerr << "[Control Thread] Invalid socket, exiting" << std::endl;
        return;
    }

    char buffer[1024];
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    // Set receive timeout so we don't block forever
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (g_running) {
        int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                        (struct sockaddr*)&clientAddr, &addrLen);

        if (n > 0) {
            buffer[n] = '\0';  // Null-terminate

            // Check if this is a HELLO packet
            if (strncmp(buffer, "HELLO", 5) == 0) {
                streamer->registerClient(clientAddr);
            }
            // Also register on any packet - makes it work even without HELLO
            else if (n > 0) {
                streamer->registerClient(clientAddr);
            }
        }

        // Periodically clean up inactive clients
        static int cleanupCounter = 0;
        if (++cleanupCounter % 10 == 0) {  // Every ~10 seconds
            streamer->cleanupInactiveClients(30);  // 30 second timeout
        }
    }

    std::cout << "[Control Thread] Stopped" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "======================================" << std::endl;
    std::cout << "CUBIT Video Streaming System" << std::endl;
    std::cout << "Low-Latency Biomedical Microscope Streaming" << std::endl;
    std::cout << "======================================" << std::endl;

    // Setup signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Load configuration file if provided
    if (argc > 1) {
        std::cout << "\n[Main] Loading config from: " << argv[1] << std::endl;
        ConfigLoader::loadFromFile(argv[1]);
    } else {
        // Try default config file
        ConfigLoader::loadFromFile("streaming.conf");
    }

    // Initialize components
    std::cout << "\n[Main] Initializing components..." << std::endl;

    // Camera
    CameraCapture camera(Config::CAMERA_DEVICE,
                        Config::CAMERA_WIDTH,
                        Config::CAMERA_HEIGHT,
                        Config::CAMERA_FPS);

    if (!camera.initialize()) {
        std::cerr << "[Main] Failed to initialize camera" << std::endl;
        std::cerr << "[Main] Make sure camera is connected and " << Config::CAMERA_DEVICE
                  << " exists" << std::endl;
        std::cerr << "[Main] You can list available cameras with: v4l2-ctl --list-devices" << std::endl;
        return 1;
    }

    // Encoder
    VideoEncoder encoder(Config::CAMERA_WIDTH,
                        Config::CAMERA_HEIGHT,
                        Config::CAMERA_FPS,
                        Config::INITIAL_BITRATE);

    if (!encoder.initialize(Config::USE_NVENC)) {
        std::cerr << "[Main] Failed to initialize encoder" << std::endl;
        camera.cleanup();
        return 1;
    }

    // Network
    UdpStreamer streamer(Config::UDP_PORT);
    if (!streamer.initialize()) {
        std::cerr << "[Main] Failed to initialize UDP streamer" << std::endl;
        encoder.cleanup();
        camera.cleanup();
        return 1;
    }

    // Monitoring
    GpuMonitor gpuMonitor;
    bool hasGpu = gpuMonitor.initialize();
    if (!hasGpu) {
        std::cout << "[Main] GPU monitoring not available (NVML init failed)" << std::endl;
        std::cout << "[Main] Adaptive bitrate will be disabled" << std::endl;
    }

    PerformanceTracker perfTracker;

    AdaptationController adaptationController(&gpuMonitor, &perfTracker, &encoder);

    // Thread-safe queues
    ThreadSafeQueue<Frame*> frameQueue(Config::CAPTURE_QUEUE_SIZE);
    ThreadSafeQueue<EncodedPacket*> packetQueue(Config::ENCODE_QUEUE_SIZE);

    std::cout << "\n[Main] Starting streaming pipeline..." << std::endl;
    std::cout << "[Main] Configuration:" << std::endl;
    std::cout << "  - Resolution: " << Config::CAMERA_WIDTH << "x" << Config::CAMERA_HEIGHT << std::endl;
    std::cout << "  - Frame rate: " << Config::CAMERA_FPS << " fps" << std::endl;
    std::cout << "  - Encoder: " << encoder.getEncoderName() << std::endl;
    std::cout << "  - Initial bitrate: " << (Config::INITIAL_BITRATE / 1000) << " kbps" << std::endl;
    std::cout << "  - UDP port: " << Config::UDP_PORT << std::endl;
    if (hasGpu) {
        std::cout << "  - GPU: " << gpuMonitor.getGpuName() << std::endl;
    }
    std::cout << "\nPress Ctrl+C to stop..." << std::endl;

    // Launch threads
    std::thread captureT(captureThread, &camera, &frameQueue, &perfTracker);
    std::thread encodingT(encodingThread, &frameQueue, &packetQueue, &encoder, &perfTracker);
    std::thread networkT(networkThread, &packetQueue, &streamer);
    std::thread controlT(controlThread, &streamer);  // Listen for client registrations

    std::thread adaptationT;
    if (hasGpu) {
        adaptationT = std::thread(adaptationThread, &adaptationController);
    }

    // Main loop - print stats periodically
    auto startTime = std::chrono::high_resolution_clock::now();
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(5));

        if (g_running) {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - startTime).count();

            std::cout << "\n[Main] === Stats (uptime: " << elapsed << "s) ===" << std::endl;
            std::cout << "  Frames captured: " << camera.getFramesCaptured() << std::endl;
            std::cout << "  Frames encoded: " << encoder.getFramesEncoded() << std::endl;
            std::cout << "  Packets sent: " << streamer.getPacketsSent() << std::endl;
            std::cout << "  Active clients: " << streamer.getActiveClients() << std::endl;
            std::cout << "  Current FPS: " << perfTracker.getCurrentFps() << std::endl;
            std::cout << "  Avg encode time: " << perfTracker.getAverageEncodeTime() << "ms" << std::endl;
            std::cout << "  Drop rate: " << (perfTracker.getFrameDropRate() * 100) << "%" << std::endl;
            std::cout << "  Current bitrate: " << (adaptationController.getCurrentBitrate() / 1000)
                      << " kbps" << std::endl;
        }
    }

    std::cout << "\n[Main] Stopping threads..." << std::endl;

    // Stop queues to unblock threads
    frameQueue.stop();
    packetQueue.stop();

    // Wait for threads to finish
    if (captureT.joinable()) captureT.join();
    if (encodingT.joinable()) encodingT.join();
    if (networkT.joinable()) networkT.join();
    if (controlT.joinable()) controlT.join();
    if (adaptationT.joinable()) adaptationT.join();

    // Cleanup
    std::cout << "[Main] Cleaning up..." << std::endl;
    streamer.cleanup();
    encoder.cleanup();
    camera.cleanup();
    if (hasGpu) {
        gpuMonitor.cleanup();
    }

    std::cout << "\n[Main] Shutdown complete. Thanks for using CUBIT Streaming!" << std::endl;
    return 0;
}
