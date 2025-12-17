#!/bin/bash
# Run script for CUBIT streaming system

set -e

echo "======================================"
echo "CUBIT Streaming System"
echo "======================================"
echo ""

# Check if binary exists
if [ ! -f "build/streaming_service" ]; then
    echo "Error: streaming_service not found"
    echo "Please build first with: ./scripts/build.sh"
    exit 1
fi

# Check if camera exists
if [ ! -e "/dev/video0" ]; then
    echo "Warning: /dev/video0 not found"
    echo "Please connect a camera or update Config::CAMERA_DEVICE in src/utils/config.h"
    echo ""
    echo "Available video devices:"
    ls -la /dev/video* 2>/dev/null || echo "  None found"
    echo ""
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Run the service
echo "Starting streaming service..."
echo ""
./build/streaming_service
