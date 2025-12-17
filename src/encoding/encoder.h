#ifndef ENCODER_H
#define ENCODER_H

#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include "../utils/frame.h"

// Encoded packet output from encoder
struct EncodedPacket {
    uint8_t* data;
    size_t size;
    int64_t pts;
    bool isKeyframe;

    EncodedPacket() : data(nullptr), size(0), pts(0), isKeyframe(false) {}

    ~EncodedPacket() {
        if (data) {
            delete[] data;
        }
    }

    // Deep copy
    void copyFrom(const uint8_t* src, size_t len) {
        if (data) {
            delete[] data;
        }
        size = len;
        data = new uint8_t[size];
        memcpy(data, src, size);
    }
};

// Video encoder using FFmpeg with NVENC support
class VideoEncoder {
public:
    VideoEncoder(int width, int height, int fps, int bitrate);
    ~VideoEncoder();

    // Initialize encoder (tries NVENC first, falls back to CPU)
    bool initialize(bool tryNvenc = true);

    // Encode a frame and return encoded packets
    // Returns number of packets generated (can be 0 if frame is buffered)
    int encodeFrame(const Frame* frame, std::vector<EncodedPacket*>& packets);

    // Flush remaining frames from encoder
    int flush(std::vector<EncodedPacket*>& packets);

    // Update bitrate dynamically
    void setBitrate(int bitrate);

    // Cleanup
    void cleanup();

    // Get encoder info
    bool isUsingHardwareAccel() const { return usingNvenc_; }
    std::string getEncoderName() const { return encoderName_; }
    int getFramesEncoded() const { return framesEncoded_; }

private:
    int width_;
    int height_;
    int fps_;
    int bitrate_;

    // FFmpeg structures
    AVCodec* codec_;
    AVCodecContext* codecCtx_;
    AVFrame* avFrame_;
    AVPacket* avPacket_;
    SwsContext* swsCtx_;  // For color space conversion

    bool initialized_;
    bool usingNvenc_;
    std::string encoderName_;
    int framesEncoded_;

    // Helper methods
    bool initializeEncoder(const char* encoderName);
    AVFrame* convertFrameToAV(const Frame* frame);
};

#endif // ENCODER_H
