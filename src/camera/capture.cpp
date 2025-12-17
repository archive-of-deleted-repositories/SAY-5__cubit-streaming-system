#include "capture.h"
#include "../utils/config.h"
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <chrono>
#include <errno.h>

CameraCapture::CameraCapture(const std::string& devicePath, int width, int height, int fps)
    : devicePath_(devicePath)
    , width_(width)
    , height_(height)
    , fps_(fps)
    , fd_(-1)
    , initialized_(false)
    , numBuffers_(Config::RING_BUFFER_SIZE)
    , framesCaptured_(0)
    , droppedFrames_(0) {

    // Pre-allocate frame buffer
    currentFrame_.width = width_;
    currentFrame_.height = height_;
    // YUV422 format: 2 bytes per pixel
    currentFrame_.allocate(width_ * height_ * 2);
}

CameraCapture::~CameraCapture() {
    cleanup();
}

bool CameraCapture::initialize() {
    std::cout << "[Camera] Initializing camera: " << devicePath_ << std::endl;
    std::cout << "[Camera] Resolution: " << width_ << "x" << height_ << " @ " << fps_ << "fps" << std::endl;

    if (!openDevice()) {
        std::cerr << "[Camera] Failed to open device" << std::endl;
        return false;
    }

    if (!initDevice()) {
        std::cerr << "[Camera] Failed to initialize device" << std::endl;
        cleanup();
        return false;
    }

    if (!initMmap()) {
        std::cerr << "[Camera] Failed to initialize memory mapping" << std::endl;
        cleanup();
        return false;
    }

    if (!startCapturing()) {
        std::cerr << "[Camera] Failed to start capturing" << std::endl;
        cleanup();
        return false;
    }

    initialized_ = true;
    std::cout << "[Camera] Initialization successful" << std::endl;
    return true;
}

bool CameraCapture::openDevice() {
    fd_ = open(devicePath_.c_str(), O_RDWR | O_NONBLOCK, 0);
    if (fd_ == -1) {
        std::cerr << "[Camera] Cannot open device: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool CameraCapture::initDevice() {
    struct v4l2_capability cap;
    if (ioctl(fd_, VIDIOC_QUERYCAP, &cap) == -1) {
        std::cerr << "[Camera] VIDIOC_QUERYCAP failed: " << strerror(errno) << std::endl;
        return false;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        std::cerr << "[Camera] Device is not a video capture device" << std::endl;
        return false;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        std::cerr << "[Camera] Device does not support streaming I/O" << std::endl;
        return false;
    }

    std::cout << "[Camera] Device: " << cap.card << std::endl;

    // Set format
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width_;
    fmt.fmt.pix.height = height_;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;  // YUV 4:2:2
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (ioctl(fd_, VIDIOC_S_FMT, &fmt) == -1) {
        std::cerr << "[Camera] VIDIOC_S_FMT failed: " << strerror(errno) << std::endl;
        return false;
    }

    // Verify format was set correctly
    if (fmt.fmt.pix.width != (unsigned)width_ || fmt.fmt.pix.height != (unsigned)height_) {
        std::cout << "[Camera] Warning: Resolution adjusted to "
                  << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height << std::endl;
        width_ = fmt.fmt.pix.width;
        height_ = fmt.fmt.pix.height;
    }

    // Set framerate
    struct v4l2_streamparm parm;
    memset(&parm, 0, sizeof(parm));
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = fps_;

    if (ioctl(fd_, VIDIOC_S_PARM, &parm) == -1) {
        std::cerr << "[Camera] Warning: Could not set framerate: " << strerror(errno) << std::endl;
        // Not fatal, continue anyway
    }

    return true;
}

bool CameraCapture::initMmap() {
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = numBuffers_;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd_, VIDIOC_REQBUFS, &req) == -1) {
        std::cerr << "[Camera] VIDIOC_REQBUFS failed: " << strerror(errno) << std::endl;
        return false;
    }

    if (req.count < 2) {
        std::cerr << "[Camera] Insufficient buffer memory" << std::endl;
        return false;
    }

    buffers_.resize(req.count);

    for (unsigned int i = 0; i < req.count; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(fd_, VIDIOC_QUERYBUF, &buf) == -1) {
            std::cerr << "[Camera] VIDIOC_QUERYBUF failed: " << strerror(errno) << std::endl;
            return false;
        }

        buffers_[i].length = buf.length;
        buffers_[i].start = mmap(NULL, buf.length,
                                 PROT_READ | PROT_WRITE,
                                 MAP_SHARED,
                                 fd_, buf.m.offset);

        if (buffers_[i].start == MAP_FAILED) {
            std::cerr << "[Camera] mmap failed: " << strerror(errno) << std::endl;
            return false;
        }
    }

    std::cout << "[Camera] Allocated " << req.count << " buffers" << std::endl;
    return true;
}

bool CameraCapture::startCapturing() {
    // Queue all buffers
    for (size_t i = 0; i < buffers_.size(); i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(fd_, VIDIOC_QBUF, &buf) == -1) {
            std::cerr << "[Camera] VIDIOC_QBUF failed: " << strerror(errno) << std::endl;
            return false;
        }
    }

    // Start streaming
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd_, VIDIOC_STREAMON, &type) == -1) {
        std::cerr << "[Camera] VIDIOC_STREAMON failed: " << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

bool CameraCapture::stopCapturing() {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd_, VIDIOC_STREAMOFF, &type) == -1) {
        std::cerr << "[Camera] VIDIOC_STREAMOFF failed: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

Frame* CameraCapture::captureFrame() {
    if (!initialized_) {
        return nullptr;
    }

    fd_set fds;
    struct timeval tv;
    int r;

    FD_ZERO(&fds);
    FD_SET(fd_, &fds);

    // Timeout after 2 seconds
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    r = select(fd_ + 1, &fds, NULL, NULL, &tv);

    if (r == -1) {
        if (errno == EINTR) {
            return nullptr;  // Interrupted, try again
        }
        std::cerr << "[Camera] select() failed: " << strerror(errno) << std::endl;
        return nullptr;
    }

    if (r == 0) {
        std::cerr << "[Camera] select() timeout" << std::endl;
        return nullptr;
    }

    // Dequeue buffer
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd_, VIDIOC_DQBUF, &buf) == -1) {
        if (errno == EAGAIN) {
            return nullptr;  // No frame ready
        }
        std::cerr << "[Camera] VIDIOC_DQBUF failed: " << strerror(errno) << std::endl;
        return nullptr;
    }

    // Get timestamp
    auto now = std::chrono::high_resolution_clock::now();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
    currentFrame_.timestamp = micros.count();
    currentFrame_.frameNumber = framesCaptured_;

    // CRITICAL: Deep copy the frame data!
    // I learned this the hard way - the camera buffer gets reused immediately
    // when we re-queue it. If we just pass a pointer to the encoder, by the
    // time it processes the frame, the data will be overwritten with the next frame.
    // This caused corrupted frames and took me 2 days to debug!
    memcpy(currentFrame_.data, buffers_[buf.index].start, buf.bytesused);
    currentFrame_.size = buf.bytesused;

    // Re-queue buffer for reuse
    if (ioctl(fd_, VIDIOC_QBUF, &buf) == -1) {
        std::cerr << "[Camera] VIDIOC_QBUF failed: " << strerror(errno) << std::endl;
    }

    framesCaptured_++;
    return &currentFrame_;
}

void CameraCapture::cleanup() {
    if (!initialized_) {
        return;
    }

    std::cout << "[Camera] Cleaning up..." << std::endl;

    stopCapturing();

    // Unmap buffers
    for (size_t i = 0; i < buffers_.size(); i++) {
        if (buffers_[i].start != nullptr && buffers_[i].start != MAP_FAILED) {
            munmap(buffers_[i].start, buffers_[i].length);
        }
    }
    buffers_.clear();

    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
    }

    initialized_ = false;
    std::cout << "[Camera] Cleanup complete. Captured " << framesCaptured_
              << " frames, dropped " << droppedFrames_ << std::endl;
}
