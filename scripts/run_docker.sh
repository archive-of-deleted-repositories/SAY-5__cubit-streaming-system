#!/bin/bash
# Docker run script for CUBIT streaming system

set -e

echo "======================================"
echo "CUBIT Streaming System - Docker"
echo "======================================"
echo ""

# Check if Docker is installed
if ! command -v docker &> /dev/null; then
    echo "Error: Docker not found"
    echo "Install from: https://docs.docker.com/get-docker/"
    exit 1
fi

# Check if nvidia-docker is available
if ! docker run --rm --gpus all nvidia/cuda:11.8.0-base-ubuntu20.04 nvidia-smi &> /dev/null; then
    echo "Warning: NVIDIA Container Toolkit not available"
    echo "GPU acceleration will not work"
    echo "Install from: https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/install-guide.html"
    echo ""
fi

# Build Docker image
echo "Building Docker image..."
docker build -t cubit-streaming:latest -f docker/Dockerfile .

echo ""
echo "Docker image built successfully"
echo ""

# Check for camera
CAMERA_DEVICE=${1:-/dev/video0}

if [ ! -e "$CAMERA_DEVICE" ]; then
    echo "Warning: Camera device $CAMERA_DEVICE not found"
    echo "Usage: $0 [camera_device]"
    echo "Example: $0 /dev/video1"
    exit 1
fi

echo "Running container with camera: $CAMERA_DEVICE"
echo ""

# Run container
# Using --rm to auto-cleanup, --gpus all for GPU, --network host for low latency
docker run --rm -it \
    --gpus all \
    --device=$CAMERA_DEVICE \
    --network host \
    --name cubit-streaming \
    cubit-streaming:latest
