#include "performance_tracker.h"
#include <numeric>
#include <algorithm>

PerformanceTracker::PerformanceTracker(int windowSize)
    : windowSize_(windowSize)
    , framesProcessed_(0)
    , framesDropped_(0)
    , framesSinceLastCheck_(0)
    , currentFps_(0.0f) {

    lastFpsCheck_ = std::chrono::high_resolution_clock::now();
}

void PerformanceTracker::recordEncodeTime(float milliseconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    addToWindow(encodeTimes_, milliseconds);
}

void PerformanceTracker::recordNetworkLatency(float milliseconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    addToWindow(networkLatencies_, milliseconds);
}

void PerformanceTracker::recordFrameDropped() {
    std::lock_guard<std::mutex> lock(mutex_);
    framesDropped_++;
}

void PerformanceTracker::recordFrameProcessed() {
    std::lock_guard<std::mutex> lock(mutex_);
    framesProcessed_++;
    framesSinceLastCheck_++;

    // Update FPS calculation every second
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastFpsCheck_).count();

    if (elapsed >= 1000) {  // 1 second
        currentFps_ = framesSinceLastCheck_ * 1000.0f / elapsed;
        framesSinceLastCheck_ = 0;
        lastFpsCheck_ = now;
    }
}

float PerformanceTracker::getAverageEncodeTime() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return calculateAverage(encodeTimes_);
}

float PerformanceTracker::getAverageNetworkLatency() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return calculateAverage(networkLatencies_);
}

float PerformanceTracker::getFrameDropRate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    int totalFrames = framesProcessed_ + framesDropped_;
    if (totalFrames == 0) {
        return 0.0f;
    }
    return static_cast<float>(framesDropped_) / totalFrames;
}

float PerformanceTracker::getCurrentFps() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentFps_;
}

void PerformanceTracker::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    encodeTimes_.clear();
    networkLatencies_.clear();
    framesProcessed_ = 0;
    framesDropped_ = 0;
    framesSinceLastCheck_ = 0;
    currentFps_ = 0.0f;
    lastFpsCheck_ = std::chrono::high_resolution_clock::now();
}

template<typename T>
void PerformanceTracker::addToWindow(std::deque<T>& window, T value) {
    window.push_back(value);
    if (static_cast<int>(window.size()) > windowSize_) {
        window.pop_front();
    }
}

float PerformanceTracker::calculateAverage(const std::deque<float>& values) const {
    if (values.empty()) {
        return 0.0f;
    }
    float sum = std::accumulate(values.begin(), values.end(), 0.0f);
    return sum / values.size();
}
