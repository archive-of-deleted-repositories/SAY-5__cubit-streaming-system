#ifndef GPU_MONITOR_H
#define GPU_MONITOR_H

#include <nvml.h>
#include <string>

// GPU monitoring using NVIDIA Management Library (NVML)
// Used to track GPU utilization for adaptive bitrate control
class GpuMonitor {
public:
    GpuMonitor();
    ~GpuMonitor();

    // Initialize NVML and get GPU handle
    bool initialize(int deviceIndex = 0);

    // Get current GPU utilization (0.0 - 1.0)
    float getUtilization();

    // Get GPU memory usage in MB
    float getMemoryUsed();

    // Get GPU temperature in Celsius
    int getTemperature();

    // Get GPU name
    std::string getGpuName();

    // Cleanup
    void cleanup();

    bool isInitialized() const { return initialized_; }

private:
    nvmlDevice_t device_;
    bool initialized_;
    std::string gpuName_;
};

#endif // GPU_MONITOR_H
