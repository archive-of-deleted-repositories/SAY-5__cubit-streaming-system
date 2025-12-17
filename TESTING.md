# Testing Guide

This document provides a step-by-step testing plan before pushing to Git.

## Pre-Push Testing Checklist

### 1. Project Structure Verification ✓

Ensure all files are in the correct locations:

```bash
# Run from project root
tree -L 3 -I 'build'

# Or use find
find . -type f ! -path './build/*' ! -path './.git/*' -name "*.cpp" -o -name "*.h"
```

### 2. Build Test (Without Camera)

Test that the project compiles:

```bash
# Clean build
rm -rf build
./scripts/build.sh

# Expected output:
# - CMake configures successfully
# - All source files compile without errors
# - Executable created at build/streaming_service

# Check executable exists
ls -lh build/streaming_service
```

### 3. Dependency Check

Verify all includes are correct:

```bash
# This should show all include dependencies
g++ -std=c++17 -I./src -MM src/main.cpp
```

### 4. Configuration File Test

Test config loading:

```bash
# Verify sample config is valid
cat streaming.conf

# Test with invalid config (should use defaults)
./build/streaming_service nonexistent.conf
# Should print: "Could not open config file" but continue
```

### 5. Code Quality Checks

Basic code quality verification:

```bash
# Check for common issues
grep -r "TODO" src/  # Review TODOs
grep -r "FIXME" src/ # Check for FIXMEs (should be none)

# Count lines of code
find src -name "*.cpp" -o -name "*.h" | xargs wc -l | tail -1
```

### 6. Documentation Verification

Ensure all docs are accurate:

```bash
# Check that README examples are correct
grep -A 5 "```bash" README.md

# Verify CHANGELOG is up to date
head -30 CHANGELOG.md

# Check all scripts are executable
ls -l scripts/*.sh
```

## Full Integration Test (With Camera)

### Option 1: With Real Camera

If you have a USB camera:

```bash
# Check camera exists
ls -l /dev/video0
v4l2-ctl --list-devices

# Run server
./build/streaming_service

# Expected output:
# - Camera initializes successfully
# - Encoder starts (NVENC or CPU fallback)
# - Network binds to port 5000
# - Control thread starts
# - All threads running

# In another terminal, test client
./scripts/client_example.sh localhost 5000

# Expected:
# - HELLO packet sent
# - Client registered on server
# - Video playback starts
```

### Option 2: Without Camera (Dry Run)

If no camera available:

```bash
# The program will fail at camera init - expected behavior
./build/streaming_service 2>&1 | head -20

# Should see:
# - Config loaded
# - Camera init attempted
# - Error: "Cannot open device /dev/video0"
# - Program exits gracefully
```

### Option 3: Simulate Camera (Advanced)

Use V4L2 loopback for testing:

```bash
# Install v4l2loopback (Ubuntu)
sudo apt-get install v4l2loopback-dkms v4l2loopback-utils

# Create virtual camera
sudo modprobe v4l2loopback

# Feed test video to virtual camera
ffmpeg -re -i test_video.mp4 -f v4l2 /dev/video0 &

# Now run streaming service
./build/streaming_service
```

## Docker Build Test

Test Docker containerization:

```bash
# Build Docker image
cd docker
docker build -t cubit-streaming:test -f Dockerfile ..

# Expected: Multi-stage build completes successfully

# Check image size
docker images | grep cubit-streaming

# Test run (will fail without camera, but checks container setup)
docker run --rm cubit-streaming:test --help 2>&1 || echo "Expected to fail without camera"
```

## Common Issues and Solutions

### Build Failures

**Issue**: CMake can't find FFmpeg
```bash
# Solution: Install FFmpeg dev libraries
sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
```

**Issue**: NVML not found
```
# This is OK - GPU monitoring is optional
# System will fall back to no adaptive bitrate
```

### Runtime Issues

**Issue**: Permission denied on /dev/video0
```bash
# Solution: Add user to video group
sudo usermod -a -G video $USER
# Then logout/login
```

**Issue**: Port 5000 already in use
```bash
# Solution: Change port in streaming.conf
echo "udp_port=5001" >> streaming.conf
```

**Issue**: Client can't connect
```bash
# Check firewall
sudo ufw status
sudo ufw allow 5000/udp

# Check server is listening
sudo netstat -nlup | grep 5000
```

## Performance Benchmarks (Optional)

If you want to measure performance:

```bash
# Monitor CPU usage
top -p $(pgrep streaming_service)

# Monitor network throughput
iftop -i eth0

# Check for dropped frames
# Look in server logs for "Dropped X frames" messages
```

## Test Summary

Before pushing to Git, verify:

- [ ] Project builds without errors
- [ ] All 18 source files compile
- [ ] Configuration file parsing works
- [ ] Documentation is accurate
- [ ] Scripts are executable
- [ ] .gitignore includes build artifacts
- [ ] No sensitive data in repository
- [ ] CHANGELOG is up to date
- [ ] README has correct usage examples

## Quick Test Script

Run this for a quick sanity check:

```bash
#!/bin/bash

echo "=== CUBIT Streaming System - Pre-Push Tests ==="
echo ""

echo "1. Checking project structure..."
if [ ! -f "CMakeLists.txt" ]; then
    echo "ERROR: Not in project root"
    exit 1
fi
echo "✓ Project root confirmed"

echo ""
echo "2. Checking all source files exist..."
FILES="
src/main.cpp
src/camera/capture.cpp
src/encoding/encoder.cpp
src/network/udp_streamer.cpp
src/monitoring/gpu_monitor.cpp
src/monitoring/performance_tracker.cpp
src/monitoring/adaptation_controller.cpp
"
for f in $FILES; do
    if [ ! -f "$f" ]; then
        echo "ERROR: Missing $f"
        exit 1
    fi
done
echo "✓ All source files present"

echo ""
echo "3. Checking documentation..."
for doc in README.md CHANGELOG.md LICENSE; do
    if [ ! -f "$doc" ]; then
        echo "ERROR: Missing $doc"
        exit 1
    fi
done
echo "✓ Documentation complete"

echo ""
echo "4. Testing build..."
./scripts/build.sh > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "✓ Build successful"
else
    echo "ERROR: Build failed"
    exit 1
fi

echo ""
echo "5. Checking executable..."
if [ -f "build/streaming_service" ]; then
    echo "✓ Executable created"
    ls -lh build/streaming_service
else
    echo "ERROR: Executable not found"
    exit 1
fi

echo ""
echo "=== All Pre-Push Tests Passed! ==="
echo "Ready to commit and push to Git"
```

Save this as `scripts/pre_push_test.sh` and run before pushing.
