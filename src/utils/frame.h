#ifndef FRAME_H
#define FRAME_H

#include <cstdint>
#include <cstring>
#include <memory>

// Represents a single video frame
// Learned the hard way - camera reuses its buffer, so we need our own copy!
struct Frame {
    uint8_t* data;          // Raw frame data (YUV or RGB depending on source)
    size_t size;            // Size of data in bytes
    int width;
    int height;
    int64_t timestamp;      // Capture timestamp in microseconds
    int frameNumber;        // Sequential frame number for debugging

    // Constructor
    Frame() : data(nullptr), size(0), width(0), height(0), timestamp(0), frameNumber(0) {}

    // Deep copy constructor - important for thread safety
    Frame(const Frame& other) {
        width = other.width;
        height = other.height;
        size = other.size;
        timestamp = other.timestamp;
        frameNumber = other.frameNumber;

        if (other.data && other.size > 0) {
            data = new uint8_t[size];
            memcpy(data, other.data, size);
        } else {
            data = nullptr;
        }
    }

    // Allocate memory for frame data
    void allocate(size_t dataSize) {
        if (data) {
            delete[] data;
        }
        size = dataSize;
        data = new uint8_t[size];
    }

    // Destructor
    ~Frame() {
        if (data) {
            delete[] data;
            data = nullptr;
        }
    }

    // Disable assignment to prevent accidental shallow copies
    Frame& operator=(const Frame&) = delete;
};

#endif // FRAME_H
