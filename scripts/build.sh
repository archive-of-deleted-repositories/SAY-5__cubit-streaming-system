#!/bin/bash
# Build script for CUBIT streaming system

set -e  # Exit on error

echo "======================================"
echo "CUBIT Streaming System - Build Script"
echo "======================================"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check for required dependencies
echo "Checking dependencies..."

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}Error: CMake not found${NC}"
    echo "Install with: sudo apt-get install cmake"
    exit 1
fi

# Check for FFmpeg development libraries
if ! pkg-config --exists libavcodec; then
    echo -e "${RED}Error: FFmpeg development libraries not found${NC}"
    echo "Install with: sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev"
    exit 1
fi

echo -e "${GREEN}✓ Dependencies OK${NC}"
echo ""

# Build type (default: Release)
BUILD_TYPE=${1:-Release}

echo "Build type: $BUILD_TYPE"
echo ""

# Create build directory
if [ -d "build" ]; then
    echo "Removing old build directory..."
    rm -rf build
fi

mkdir build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE

# Build
echo ""
echo "Building..."
make -j$(nproc)

echo ""
echo -e "${GREEN}======================================"
echo "Build completed successfully!"
echo "======================================${NC}"
echo ""
echo "Run with: ./build/streaming_service"
echo "Or use: ./scripts/run.sh"
echo ""
