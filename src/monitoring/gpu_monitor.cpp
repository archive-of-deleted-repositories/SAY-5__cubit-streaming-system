#include "gpu_monitor.h"
#include <iostream>
#include <cstring>

GpuMonitor::GpuMonitor()
    : device_(nullptr)
    , initialized_(false) {
}

GpuMonitor::~GpuMonitor() {
    cleanup();
}

bool GpuMonitor::initialize(int deviceIndex) {
    std::cout << "[GPU Monitor] Initializing NVML..." << std::endl;

    nvmlReturn_t result = nvmlInit();
    if (result != NVML_SUCCESS) {
        std::cerr << "[GPU Monitor] Failed to initialize NVML: "
                  << nvmlErrorString(result) << std::endl;
        return false;
    }

    result = nvmlDeviceGetHandleByIndex(deviceIndex, &device_);
    if (result != NVML_SUCCESS) {
        std::cerr << "[GPU Monitor] Failed to get device handle: "
                  << nvmlErrorString(result) << std::endl;
        nvmlShutdown();
        return false;
    }

    // Get GPU name
    char name[NVML_DEVICE_NAME_BUFFER_SIZE];
    result = nvmlDeviceGetName(device_, name, NVML_DEVICE_NAME_BUFFER_SIZE);
    if (result == NVML_SUCCESS) {
        gpuName_ = std::string(name);
        std::cout << "[GPU Monitor] Monitoring GPU: " << gpuName_ << std::endl;
    }

    initialized_ = true;
    std::cout << "[GPU Monitor] NVML initialized successfully" << std::endl;
    return true;
}

float GpuMonitor::getUtilization() {
    if (!initialized_) {
        return 0.0f;
    }

    nvmlUtilization_t utilization;
    nvmlReturn_t result = nvmlDeviceGetUtilizationRates(device_, &utilization);

    if (result != NVML_SUCCESS) {
        // Don't spam errors - just return 0
        return 0.0f;
    }

    // Returns GPU utilization as percentage (0-100), convert to 0.0-1.0
    return utilization.gpu / 100.0f;
}

float GpuMonitor::getMemoryUsed() {
    if (!initialized_) {
        return 0.0f;
    }

    nvmlMemory_t memory;
    nvmlReturn_t result = nvmlDeviceGetMemoryInfo(device_, &memory);

    if (result != NVML_SUCCESS) {
        return 0.0f;
    }

    // Return used memory in MB
    return memory.used / (1024.0f * 1024.0f);
}

int GpuMonitor::getTemperature() {
    if (!initialized_) {
        return 0;
    }

    unsigned int temp;
    nvmlReturn_t result = nvmlDeviceGetTemperature(device_, NVML_TEMPERATURE_GPU, &temp);

    if (result != NVML_SUCCESS) {
        return 0;
    }

    return static_cast<int>(temp);
}

std::string GpuMonitor::getGpuName() {
    return gpuName_;
}

void GpuMonitor::cleanup() {
    if (!initialized_) {
        return;
    }

    std::cout << "[GPU Monitor] Shutting down NVML..." << std::endl;
    nvmlShutdown();
    initialized_ = false;
}
