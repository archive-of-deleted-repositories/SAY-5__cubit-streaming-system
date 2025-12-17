#ifndef CAPTURE_H
#define CAPTURE_H

#include <string>
#include <vector>
#include "../utils/frame.h"

// Camera capture using V4L2 (Video4Linux2) API
class CameraCapture {
public:
    CameraCapture(const std::string& devicePath, int width, int height, int fps);
    ~CameraCapture();

    // Initialize camera and allocate buffers
    bool initialize();

    // Capture a single frame (blocking)
    // Returns pointer to internal buffer - caller must COPY data immediately!
    Frame* captureFrame();

    // Cleanup and release resources
    void cleanup();

    // Check if camera is ready
    bool isReady() const { return initialized_; }

    // Get stats
    int getFramesCaptured() const { return framesCaptured_; }
    int getDroppedFrames() const { return droppedFrames_; }

private:
    std::string devicePath_;
    int width_;
    int height_;
    int fps_;
    int fd_;  // File descriptor for camera device
    bool initialized_;

    // V4L2 buffer management
    struct BufferInfo {
        void* start;
        size_t length;
    };
    std::vector<BufferInfo> buffers_;
    int numBuffers_;

    // Frame tracking
    int framesCaptured_;
    int droppedFrames_;
    Frame currentFrame_;  // Reused frame structure to avoid allocations

    // Helper methods
    bool openDevice();
    bool initDevice();
    bool initMmap();
    bool startCapturing();
    bool stopCapturing();
};

#endif // CAPTURE_H
