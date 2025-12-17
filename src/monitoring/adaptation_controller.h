#ifndef ADAPTATION_CONTROLLER_H
#define ADAPTATION_CONTROLLER_H

#include "gpu_monitor.h"
#include "performance_tracker.h"
#include "../encoding/encoder.h"

// Controls adaptive bitrate based on GPU utilization and performance metrics
// The goal is to maximize quality while maintaining target latency
class AdaptationController {
public:
    AdaptationController(GpuMonitor* gpuMonitor,
                        PerformanceTracker* perfTracker,
                        VideoEncoder* encoder);

    // Check if bitrate adjustment is needed and apply it
    // Should be called periodically (e.g., every 2 seconds)
    void update();

    // Get current bitrate
    int getCurrentBitrate() const { return currentBitrate_; }

    // Manual bitrate override
    void setTargetBitrate(int bitrate);

private:
    GpuMonitor* gpuMonitor_;
    PerformanceTracker* perfTracker_;
    VideoEncoder* encoder_;

    int currentBitrate_;
    int targetBitrate_;

    // Hysteresis to prevent oscillation
    // I added this after noticing bitrate was changing too frequently
    int consecutiveHighUtil_;
    int consecutiveLowUtil_;
    const int HYSTERESIS_THRESHOLD = 3;  // Need 3 consecutive readings

    // Helper to calculate new bitrate
    int calculateNewBitrate(float gpuUtil, float avgEncodeTime);
};

#endif // ADAPTATION_CONTROLLER_H
