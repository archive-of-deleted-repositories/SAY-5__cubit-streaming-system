#include "adaptation_controller.h"
#include "../utils/config.h"
#include <iostream>
#include <algorithm>

AdaptationController::AdaptationController(GpuMonitor* gpuMonitor,
                                          PerformanceTracker* perfTracker,
                                          VideoEncoder* encoder)
    : gpuMonitor_(gpuMonitor)
    , perfTracker_(perfTracker)
    , encoder_(encoder)
    , currentBitrate_(Config::INITIAL_BITRATE)
    , targetBitrate_(Config::INITIAL_BITRATE)
    , consecutiveHighUtil_(0)
    , consecutiveLowUtil_(0) {
}

void AdaptationController::update() {
    if (!gpuMonitor_ || !gpuMonitor_->isInitialized()) {
        // Can't adapt without GPU monitoring
        return;
    }

    float gpuUtil = gpuMonitor_->getUtilization();
    float avgEncodeTime = perfTracker_->getAverageEncodeTime();

    // Decision logic with hysteresis to prevent oscillation
    bool shouldDecrease = false;
    bool shouldIncrease = false;

    // Check if GPU is overloaded
    if (gpuUtil > Config::GPU_HIGH_UTIL_THRESHOLD) {
        consecutiveHighUtil_++;
        consecutiveLowUtil_ = 0;

        if (consecutiveHighUtil_ >= HYSTERESIS_THRESHOLD) {
            shouldDecrease = true;
        }
    }
    // Check if we can increase quality
    else if (gpuUtil < Config::GPU_LOW_UTIL_THRESHOLD &&
             avgEncodeTime < Config::TARGET_ENCODE_LATENCY_MS) {
        consecutiveLowUtil_++;
        consecutiveHighUtil_ = 0;

        if (consecutiveLowUtil_ >= HYSTERESIS_THRESHOLD) {
            shouldIncrease = true;
        }
    }
    // In the middle - reset counters
    else {
        consecutiveHighUtil_ = 0;
        consecutiveLowUtil_ = 0;
    }

    // Apply bitrate adjustment
    int newBitrate = currentBitrate_;

    if (shouldDecrease) {
        // Decrease bitrate by 20%
        newBitrate = static_cast<int>(currentBitrate_ * Config::BITRATE_ADJUSTMENT_FACTOR);
        std::cout << "[Adaptation] GPU utilization high (" << (gpuUtil * 100) << "%), "
                  << "decreasing bitrate" << std::endl;
        consecutiveHighUtil_ = 0;  // Reset after adjusting
    }
    else if (shouldIncrease && currentBitrate_ < Config::MAX_BITRATE) {
        // Increase bitrate by 25% (inverse of 0.8)
        newBitrate = static_cast<int>(currentBitrate_ / Config::BITRATE_ADJUSTMENT_FACTOR);
        std::cout << "[Adaptation] GPU utilization low (" << (gpuUtil * 100) << "%), "
                  << "increasing bitrate" << std::endl;
        consecutiveLowUtil_ = 0;  // Reset after adjusting
    }

    // Clamp to min/max
    newBitrate = std::max(Config::MIN_BITRATE, std::min(Config::MAX_BITRATE, newBitrate));

    // Apply if changed
    if (newBitrate != currentBitrate_) {
        currentBitrate_ = newBitrate;
        encoder_->setBitrate(currentBitrate_);

        // Log the change
        std::cout << "[Adaptation] Bitrate adjusted to " << (currentBitrate_ / 1000)
                  << " kbps (GPU: " << (gpuUtil * 100) << "%, Encode: "
                  << avgEncodeTime << "ms)" << std::endl;
    }

    // Periodic status update
    static int updateCount = 0;
    updateCount++;
    if (updateCount % 10 == 0) {  // Every ~20 seconds
        std::cout << "[Adaptation] Status - Bitrate: " << (currentBitrate_ / 1000)
                  << " kbps, GPU: " << (gpuUtil * 100) << "%, Encode time: "
                  << avgEncodeTime << "ms, FPS: " << perfTracker_->getCurrentFps() << std::endl;
    }
}

void AdaptationController::setTargetBitrate(int bitrate) {
    targetBitrate_ = std::max(Config::MIN_BITRATE, std::min(Config::MAX_BITRATE, bitrate));
    currentBitrate_ = targetBitrate_;
    encoder_->setBitrate(currentBitrate_);
    std::cout << "[Adaptation] Manual bitrate set to " << (currentBitrate_ / 1000) << " kbps" << std::endl;
}
