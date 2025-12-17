cmake_minimum_required(VERSION 3.15)
project(cubit_streaming_system VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Find required packages
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

# FFmpeg libraries
# Spent some time figuring out the right components - these are the essentials
pkg_check_modules(LIBAV REQUIRED
    libavcodec
    libavformat
    libavutil
    libswscale
)

# CUDA (optional for NVENC)
find_package(CUDA QUIET)
if(CUDA_FOUND)
    message(STATUS "CUDA found: ${CUDA_VERSION}")
    include_directories(${CUDA_INCLUDE_DIRS})
    add_definitions(-DHAVE_CUDA)
else()
    message(STATUS "CUDA not found - NVENC will not be available")
endif()

# NVML (NVIDIA Management Library)
# Had to manually specify paths since it doesn't have a CMake package
set(NVML_INCLUDE_DIR "/usr/local/cuda/include" CACHE PATH "NVML include directory")
set(NVML_LIBRARY "/usr/local/cuda/lib64/stubs/libnvidia-ml.so" CACHE FILEPATH "NVML library")

# Check if NVML is available
if(EXISTS ${NVML_INCLUDE_DIR}/nvml.h)
    message(STATUS "NVML found: ${NVML_LIBRARY}")
    add_definitions(-DHAVE_NVML)
    set(NVML_FOUND TRUE)
else()
    message(STATUS "NVML not found - GPU monitoring will not be available")
    message(STATUS "  Looked in: ${NVML_INCLUDE_DIR}")
    message(STATUS "  Install CUDA toolkit to enable GPU monitoring")
    set(NVML_FOUND FALSE)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${LIBAV_INCLUDE_DIRS}
)

if(NVML_FOUND)
    include_directories(${NVML_INCLUDE_DIR})
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/camera/capture.cpp
    src/encoding/encoder.cpp
    src/network/udp_streamer.cpp
    src/monitoring/performance_tracker.cpp
    src/monitoring/adaptation_controller.cpp
)

# Add GPU monitor only if NVML is available
if(NVML_FOUND)
    list(APPEND SOURCES src/monitoring/gpu_monitor.cpp)
endif()

# Executable
add_executable(streaming_service ${SOURCES})

# Link libraries
target_link_libraries(streaming_service
    Threads::Threads
    ${LIBAV_LIBRARIES}
)

if(NVML_FOUND)
    target_link_libraries(streaming_service ${NVML_LIBRARY})
endif()

# Set library search paths
target_link_directories(streaming_service PRIVATE
    ${LIBAV_LIBRARY_DIRS}
)

# Installation
install(TARGETS streaming_service
    RUNTIME DESTINATION bin
)

# Print configuration summary
message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  FFmpeg: ${LIBAV_VERSION}")
message(STATUS "  CUDA: ${CUDA_FOUND}")
message(STATUS "  NVML: ${NVML_FOUND}")
message(STATUS "")
