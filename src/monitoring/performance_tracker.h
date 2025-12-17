#ifndef PERFORMANCE_TRACKER_H
#define PERFORMANCE_TRACKER_H

#include <chrono>
#include <deque>
#include <mutex>

// Tracks encoding and network performance metrics
// Uses a sliding window to calculate averages
class PerformanceTracker {
public:
    PerformanceTracker(int windowSize = 60);  // Default: 60 samples (1 sec at 60fps)

    // Record timing measurements
    void recordEncodeTime(float milliseconds);
    void recordNetworkLatency(float milliseconds);
    void recordFrameDropped();

    // Get statistics
    float getAverageEncodeTime() const;
    float getAverageNetworkLatency() const;
    float getFrameDropRate() const;  // Fraction of frames dropped (0.0 - 1.0)

    // Get current FPS
    void recordFrameProcessed();
    float getCurrentFps() const;

    // Reset statistics
    void reset();

private:
    int windowSize_;
    mutable std::mutex mutex_;

    // Circular buffers for timing data
    std::deque<float> encodeTimes_;
    std::deque<float> networkLatencies_;

    // Frame tracking
    int framesProcessed_;
    int framesDropped_;
    std::chrono::high_resolution_clock::time_point lastFpsCheck_;
    int framesSinceLastCheck_;

    float currentFps_;

    // Helper to maintain window size
    template<typename T>
    void addToWindow(std::deque<T>& window, T value);

    float calculateAverage(const std::deque<float>& values) const;
};

#endif // PERFORMANCE_TRACKER_H
