#ifndef CONFIG_H
#define CONFIG_H

#include <string>

// Configuration for the streaming system
// Fixed: Changed to inline variables so config can be loaded from file
// See config_loader.h for file-based configuration
namespace Config {
    // Camera settings
    inline int CAMERA_WIDTH = 1920;
    inline int CAMERA_HEIGHT = 1080;
    inline int CAMERA_FPS = 60;
    inline std::string CAMERA_DEVICE = "/dev/video0";  // Default V4L2 device

    // Encoding settings
    inline int INITIAL_BITRATE = 5000000;  // 5 Mbps - settled on this after testing
    inline int MIN_BITRATE = 2000000;      // 2 Mbps minimum
    inline int MAX_BITRATE = 10000000;     // 10 Mbps maximum
    inline std::string ENCODER_PRESET = "fast";  // Tried "ultrafast" but quality was too poor
    const int GOP_SIZE = 60;              // 1 keyframe per second at 60fps
    inline bool USE_NVENC = true;          // Try NVENC first, fallback to CPU if unavailable

    // Network settings
    inline int UDP_PORT = 5000;
    const std::string STREAM_ADDRESS = "0.0.0.0";  // Listen on all interfaces
    const int MAX_PACKET_SIZE = 1400;     // MTU-safe packet size

    // Performance targets
    const int TARGET_CAPTURE_LATENCY_MS = 5;
    const int TARGET_ENCODE_LATENCY_MS = 10;
    const int RING_BUFFER_SIZE = 5;       // Number of frames in capture buffer

    // Adaptive bitrate thresholds
    const float GPU_HIGH_UTIL_THRESHOLD = 0.85f;   // 85% - decrease bitrate
    const float GPU_LOW_UTIL_THRESHOLD = 0.60f;    // 60% - consider increasing bitrate
    const int ADAPTATION_INTERVAL_MS = 2000;       // Check every 2 seconds
    const float BITRATE_ADJUSTMENT_FACTOR = 0.8f;  // Adjust by 20% each time

    // Thread queue sizes
    inline int CAPTURE_QUEUE_SIZE = 10;
    inline int ENCODE_QUEUE_SIZE = 10;
}

#endif // CONFIG_H
