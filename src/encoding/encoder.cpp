#include "encoder.h"
#include "../utils/config.h"
#include <iostream>
#include <chrono>

VideoEncoder::VideoEncoder(int width, int height, int fps, int bitrate)
    : width_(width)
    , height_(height)
    , fps_(fps)
    , bitrate_(bitrate)
    , codec_(nullptr)
    , codecCtx_(nullptr)
    , avFrame_(nullptr)
    , avPacket_(nullptr)
    , swsCtx_(nullptr)
    , initialized_(false)
    , usingNvenc_(false)
    , framesEncoded_(0) {
}

VideoEncoder::~VideoEncoder() {
    cleanup();
}

bool VideoEncoder::initialize(bool tryNvenc) {
    std::cout << "[Encoder] Initializing video encoder..." << std::endl;
    std::cout << "[Encoder] Target: " << width_ << "x" << height_
              << " @ " << fps_ << "fps, " << bitrate_/1000 << " kbps" << std::endl;

    // Try NVENC first if requested
    if (tryNvenc) {
        std::cout << "[Encoder] Attempting NVENC hardware encoding..." << std::endl;
        if (initializeEncoder("h264_nvenc")) {
            usingNvenc_ = true;
            encoderName_ = "h264_nvenc";
            std::cout << "[Encoder] Successfully initialized NVENC encoder" << std::endl;
            return true;
        }
        std::cout << "[Encoder] NVENC not available, falling back to CPU encoding" << std::endl;
    }

    // Fallback to software encoding
    std::cout << "[Encoder] Using CPU encoder (libx264)..." << std::endl;
    if (initializeEncoder("libx264")) {
        usingNvenc_ = false;
        encoderName_ = "libx264";
        std::cout << "[Encoder] Successfully initialized CPU encoder" << std::endl;
        return true;
    }

    std::cerr << "[Encoder] Failed to initialize any encoder!" << std::endl;
    return false;
}

bool VideoEncoder::initializeEncoder(const char* encoderName) {
    codec_ = avcodec_find_encoder_by_name(encoderName);
    if (!codec_) {
        std::cerr << "[Encoder] Codec " << encoderName << " not found" << std::endl;
        return false;
    }

    codecCtx_ = avcodec_alloc_context3(codec_);
    if (!codecCtx_) {
        std::cerr << "[Encoder] Could not allocate codec context" << std::endl;
        return false;
    }

    // Set encoding parameters
    codecCtx_->width = width_;
    codecCtx_->height = height_;
    codecCtx_->time_base = {1, fps_};
    codecCtx_->framerate = {fps_, 1};
    codecCtx_->bit_rate = bitrate_;
    codecCtx_->gop_size = Config::GOP_SIZE;  // 1 keyframe per second
    codecCtx_->max_b_frames = 0;  // No B-frames for low latency!
    codecCtx_->pix_fmt = AV_PIX_FMT_YUV420P;

    // NVENC specific settings
    if (strcmp(encoderName, "h264_nvenc") == 0) {
        av_opt_set(codecCtx_->priv_data, "preset", "p4", 0);  // p4 = fast preset
        av_opt_set(codecCtx_->priv_data, "tune", "ll", 0);    // ll = low latency
        av_opt_set(codecCtx_->priv_data, "zerolatency", "1", 0);
        av_opt_set(codecCtx_->priv_data, "delay", "0", 0);
    }
    // libx264 specific settings
    else if (strcmp(encoderName, "libx264") == 0) {
        // Started with "ultrafast" but quality was terrible for microscope images
        // Settled on "fast" - adds ~3ms encode time but quality is acceptable
        av_opt_set(codecCtx_->priv_data, "preset", Config::ENCODER_PRESET.c_str(), 0);
        av_opt_set(codecCtx_->priv_data, "tune", "zerolatency", 0);
        av_opt_set(codecCtx_->priv_data, "profile", "baseline", 0);
    }

    // Open codec
    if (avcodec_open2(codecCtx_, codec_, nullptr) < 0) {
        std::cerr << "[Encoder] Could not open codec " << encoderName << std::endl;
        avcodec_free_context(&codecCtx_);
        return false;
    }

    // Allocate frame
    avFrame_ = av_frame_alloc();
    if (!avFrame_) {
        std::cerr << "[Encoder] Could not allocate frame" << std::endl;
        avcodec_free_context(&codecCtx_);
        return false;
    }

    avFrame_->format = codecCtx_->pix_fmt;
    avFrame_->width = width_;
    avFrame_->height = height_;

    if (av_frame_get_buffer(avFrame_, 0) < 0) {
        std::cerr << "[Encoder] Could not allocate frame data" << std::endl;
        av_frame_free(&avFrame_);
        avcodec_free_context(&codecCtx_);
        return false;
    }

    // Allocate packet
    avPacket_ = av_packet_alloc();
    if (!avPacket_) {
        std::cerr << "[Encoder] Could not allocate packet" << std::endl;
        av_frame_free(&avFrame_);
        avcodec_free_context(&codecCtx_);
        return false;
    }

    // Initialize color space converter (YUYV to YUV420P)
    swsCtx_ = sws_getContext(
        width_, height_, AV_PIX_FMT_YUYV422,     // Source: camera format
        width_, height_, AV_PIX_FMT_YUV420P,     // Dest: encoder format
        SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!swsCtx_) {
        std::cerr << "[Encoder] Could not initialize color space converter" << std::endl;
        av_packet_free(&avPacket_);
        av_frame_free(&avFrame_);
        avcodec_free_context(&codecCtx_);
        return false;
    }

    initialized_ = true;
    return true;
}

int VideoEncoder::encodeFrame(const Frame* frame, std::vector<EncodedPacket*>& packets) {
    if (!initialized_ || !frame) {
        return 0;
    }

    auto encodeStart = std::chrono::high_resolution_clock::now();

    // Convert YUYV to YUV420P
    const uint8_t* srcData[1] = { frame->data };
    int srcLinesize[1] = { width_ * 2 };  // YUYV is 2 bytes per pixel

    if (av_frame_make_writable(avFrame_) < 0) {
        std::cerr << "[Encoder] Frame not writable" << std::endl;
        return 0;
    }

    sws_scale(swsCtx_, srcData, srcLinesize, 0, height_,
              avFrame_->data, avFrame_->linesize);

    avFrame_->pts = framesEncoded_;

    // Send frame to encoder
    int ret = avcodec_send_frame(codecCtx_, avFrame_);
    if (ret < 0) {
        std::cerr << "[Encoder] Error sending frame: " << ret << std::endl;
        return 0;
    }

    // Receive encoded packets
    int packetCount = 0;
    while (ret >= 0) {
        ret = avcodec_receive_packet(codecCtx_, avPacket_);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;  // Need more frames or done
        }

        if (ret < 0) {
            std::cerr << "[Encoder] Error receiving packet: " << ret << std::endl;
            break;
        }

        // Create encoded packet
        EncodedPacket* packet = new EncodedPacket();
        packet->copyFrom(avPacket_->data, avPacket_->size);
        packet->pts = avPacket_->pts;
        packet->isKeyframe = (avPacket_->flags & AV_PKT_FLAG_KEY) != 0;

        packets.push_back(packet);
        packetCount++;

        av_packet_unref(avPacket_);
    }

    framesEncoded_++;

    auto encodeEnd = std::chrono::high_resolution_clock::now();
    auto encodeTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        encodeEnd - encodeStart).count();

    // Log performance occasionally
    if (framesEncoded_ % 300 == 0) {  // Every 5 seconds at 60fps
        std::cout << "[Encoder] Frame " << framesEncoded_
                  << " encoded in " << encodeTime << "ms"
                  << " (using " << encoderName_ << ")" << std::endl;
    }

    return packetCount;
}

int VideoEncoder::flush(std::vector<EncodedPacket*>& packets) {
    if (!initialized_) {
        return 0;
    }

    // Send null frame to flush encoder
    avcodec_send_frame(codecCtx_, nullptr);

    int packetCount = 0;
    int ret = 0;

    while (ret >= 0) {
        ret = avcodec_receive_packet(codecCtx_, avPacket_);

        if (ret == AVERROR_EOF) {
            break;
        }

        if (ret < 0) {
            break;
        }

        EncodedPacket* packet = new EncodedPacket();
        packet->copyFrom(avPacket_->data, avPacket_->size);
        packet->pts = avPacket_->pts;
        packet->isKeyframe = (avPacket_->flags & AV_PKT_FLAG_KEY) != 0;

        packets.push_back(packet);
        packetCount++;

        av_packet_unref(avPacket_);
    }

    return packetCount;
}

void VideoEncoder::setBitrate(int bitrate) {
    if (!initialized_ || bitrate == bitrate_) {
        return;
    }

    std::cout << "[Encoder] Adjusting bitrate: " << bitrate_/1000
              << " -> " << bitrate/1000 << " kbps" << std::endl;

    bitrate_ = bitrate;
    codecCtx_->bit_rate = bitrate_;

    // For NVENC, we can update bitrate on the fly
    if (usingNvenc_) {
        // This works for some encoders but not all - best effort
        av_opt_set_int(codecCtx_->priv_data, "b", bitrate_, 0);
    }
    // For CPU encoding, bitrate change takes effect gradually
}

void VideoEncoder::cleanup() {
    if (!initialized_) {
        return;
    }

    std::cout << "[Encoder] Cleaning up encoder..." << std::endl;

    if (swsCtx_) {
        sws_freeContext(swsCtx_);
        swsCtx_ = nullptr;
    }

    if (avPacket_) {
        av_packet_free(&avPacket_);
    }

    if (avFrame_) {
        av_frame_free(&avFrame_);
    }

    if (codecCtx_) {
        avcodec_free_context(&codecCtx_);
    }

    initialized_ = false;
    std::cout << "[Encoder] Cleanup complete. Encoded " << framesEncoded_ << " frames" << std::endl;
}
