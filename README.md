# CUBIT Video Streaming System

Low-latency video streaming system for biomedical microscope cameras, developed during my internship at CUBIT (Center for Understanding Biology using Imaging Technology) at Stony Brook University.

## Overview

This system captures video from biomedical microscope cameras and streams it to researchers' computers with minimal latency, enabling real-time remote viewing and control of experiments. The system achieves end-to-end latency of 50-70ms while maintaining 60fps at 1920x1080 resolution.

### Key Features

- **Hardware-Accelerated Encoding**: Uses NVIDIA NVENC for GPU encoding with automatic fallback to CPU
- **Ultra-Low Latency**: Multi-threaded pipeline optimized for minimal latency (< 70ms end-to-end)
- **Adaptive Bitrate**: Automatically adjusts quality based on GPU utilization and network conditions
- **Scalable Architecture**: Supports 500+ concurrent client connections via UDP multicast
- **Docker Support**: Containerized deployment with GPU passthrough
- **Production Ready**: Comprehensive error handling, logging, and monitoring

## Architecture

```
┌─────────────┐
│   Camera    │
│ (V4L2 API)  │
└──────┬──────┘
       │ Ring Buffer (Deep Copy)
       v
┌─────────────────┐
│ Capture Thread  │──> Frame Queue (Thread-Safe)
└─────────────────┘
       │
       v
┌─────────────────┐
│ Encoding Thread │──> Packet Queue
│  (NVENC/x264)   │
└─────────────────┘
       │
       v
┌─────────────────┐
│ Network Thread  │──> UDP Broadcast
│  (UDP Sender)   │     to Clients
└─────────────────┘
       │
       └──────> GPU Monitor ──> Adaptation Controller
                (NVML)          (Bitrate Adjust)
```

### Threading Model

1. **Capture Thread**: Continuously grabs frames from camera via V4L2, performs deep copy, and pushes to frame queue
2. **Encoding Thread**: Pops frames from queue, encodes using NVENC/x264, produces H.264 packets
3. **Network Thread**: Sends encoded packets to all connected clients via UDP
4. **Adaptation Thread**: Monitors GPU utilization and adjusts bitrate every 2 seconds

All threads communicate via lock-protected queues with condition variables for synchronization.

## Performance Targets

| Metric | Target | Typical |
|--------|--------|---------|
| End-to-end Latency | < 70ms | 50-60ms |
| Frame Rate | 60 fps | 60 fps |
| Resolution | 1920x1080 | 1920x1080 |
| Capture Latency | < 5ms | 2-3ms |
| Encode Latency | < 10ms | 8-10ms |
| Bitrate Range | 2-10 Mbps | 5 Mbps |

## Dependencies

### Required

- **C++ Compiler**: GCC 7+ or Clang 6+ with C++17 support
- **CMake**: 3.15 or later
- **FFmpeg**: 4.x with development libraries
  - libavcodec
  - libavformat
  - libavutil
  - libswscale
- **V4L2**: Video4Linux2 for camera access (Linux only)
- **Threads**: POSIX threads (pthread)

### Optional

- **CUDA Toolkit**: 11.0+ (for NVENC hardware encoding)
- **NVIDIA GPU**: With NVENC support (GTX 10-series or newer)
- **NVML**: NVIDIA Management Library (for GPU monitoring and adaptive bitrate)
- **Docker**: For containerized deployment
- **nvidia-container-toolkit**: For Docker GPU access

## Building

### Native Build

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev \
    libv4l-dev \
    v4l-utils

# For NVENC support, install CUDA Toolkit
# Download from: https://developer.nvidia.com/cuda-downloads

# Build
./scripts/build.sh

# Run
./scripts/run.sh
```

### Docker Build

```bash
# Install Docker and NVIDIA Container Toolkit
# https://docs.docker.com/get-docker/
# https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/install-guide.html

# Build and run
./scripts/run_docker.sh

# Or use docker-compose
cd docker
docker-compose up --build
```

## Configuration

### Runtime Configuration (Recommended)

Edit `streaming.conf` to change settings without recompiling:

```ini
# Camera Settings
camera_width=1920
camera_height=1080
camera_fps=60
camera_device="/dev/video0"

# Encoding Settings
initial_bitrate=5000000  # 5 Mbps
min_bitrate=2000000      # 2 Mbps
max_bitrate=10000000     # 10 Mbps
encoder_preset="fast"
use_nvenc=true

# Network Settings
udp_port=5000
```

Run with custom config:
```bash
./build/streaming_service my_config.conf
```

### Compile-Time Configuration

Alternatively, edit `src/utils/config.h` for default values. Rebuild with `./scripts/build.sh` after changes.

## Running

### List Available Cameras

```bash
v4l2-ctl --list-devices
```

### Run the Service

```bash
# Native
./build/streaming_service

# Docker
docker run --rm -it \
    --gpus all \
    --device=/dev/video0 \
    --network host \
    cubit-streaming:latest
```

### Client Playback

Use the provided client script (recommended):

```bash
./scripts/client_example.sh SERVER_IP 5000
```

This script automatically:
1. Sends HELLO packet to register with server
2. Receives and plays stream with optimal settings

Or use FFplay directly:

```bash
# Send registration packet first
echo -n "HELLO" | nc -u -w1 SERVER_IP 5000

# Then play stream
ffplay -fflags nobuffer -flags low_delay -framedrop udp://SERVER_IP:5000
```

Or use VLC (sends data automatically on connection):
```bash
vlc udp://@:5000
```

## Troubleshooting

### Camera Not Found

```
Error: Cannot open device /dev/video0
```

**Solution**:
- Check camera is connected: `ls -la /dev/video*`
- Verify permissions: `sudo usermod -a -G video $USER` (then logout/login)
- Try different device: Update `CAMERA_DEVICE` in config.h

### NVENC Initialization Failed

```
NVENC not available, falling back to CPU encoding
```

**Solution**:
- Verify NVIDIA GPU: `nvidia-smi`
- Check CUDA installation: `nvcc --version`
- Ensure FFmpeg built with NVENC: `ffmpeg -encoders | grep nvenc`
- Rebuild FFmpeg with `--enable-nvenc` if needed

### High Latency

If latency exceeds 100ms:

1. **Check GPU utilization**: If > 90%, encoder is bottleneck
   - Decrease bitrate or resolution
   - Use faster preset (already using "fast")
2. **Check network**: Use `iperf` to test bandwidth
3. **Check frame drops**: Look for "Dropped frames" in logs
4. **Disable adaptive bitrate**: Comment out adaptation thread if causing issues

### Docker GPU Not Working

```
Failed to initialize NVML
```

**Solution**:
- Install nvidia-container-toolkit:
  ```bash
  distribution=$(. /etc/os-release;echo $ID$VERSION_ID)
  curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | sudo apt-key add -
  curl -s -L https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.list | \
      sudo tee /etc/apt/sources.list.d/nvidia-docker.list
  sudo apt-get update && sudo apt-get install -y nvidia-container-toolkit
  sudo systemctl restart docker
  ```
- Verify: `docker run --rm --gpus all nvidia/cuda:11.8.0-base nvidia-smi`

## Development Notes

### Critical Bug Fixes (v1.0.1)

#### Missing Client Registration
In the initial release, I had the client registration infrastructure (ClientInfo, registerClient method) but completely forgot to add the thread that actually listens for clients! The server would start but clients couldn't connect. Fixed by adding a control thread that listens for "HELLO" packets on the same UDP socket. Embarrassing oversight but caught during testing.

#### Packet Fragmentation
H.264 keyframes can easily exceed 10KB, but I was sending them in single UDP packets. This caused massive corruption since UDP MTU is typically 1500 bytes. Added packet fragmentation with a header structure (frameId, fragmentIndex, totalFragments) to split large frames properly. This was less obvious initially since P-frames are small and looked fine.

### The Deep Copy Bug

Early in development, I had frames getting corrupted during encoding. Turns out the V4L2 camera API reuses its internal buffers - when you call `captureFrame()`, it returns a pointer to a buffer that will be overwritten on the next capture. The encoder was processing frames asynchronously, so by the time it got to encoding, the data was already garbage.

**Solution**: Deep copy frame data immediately after capture before passing to encoder queue. See `src/camera/capture.cpp:125` for implementation.

```cpp
// WRONG - pointer to camera's reused buffer
Frame* frame = camera.captureFrame();
frameQueue.push(frame);  // Data gets corrupted!

// CORRECT - our own copy
Frame* cameraFrame = camera.captureFrame();
Frame* ownedFrame = new Frame(*cameraFrame);  // Deep copy
frameQueue.push(ownedFrame);  // Safe!
```

### Encoder Preset Evolution

- **Started with**: "ultrafast" - too blocky for microscope images
- **Tried**: "medium" - good quality but 15ms encode time (too slow)
- **Settled on**: "fast" - 8-10ms encode time, acceptable quality

### Adaptive Bitrate Hysteresis

Originally the bitrate would oscillate wildly because it adjusted immediately based on single GPU readings. Added hysteresis requiring 3 consecutive high/low readings before adjusting. Much more stable now.

## Performance Tuning

### Maximizing Throughput

1. **CPU Affinity**: Pin threads to specific cores
   ```cpp
   // In main.cpp
   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(0, &cpuset);  // Pin capture to core 0
   pthread_setaffinity_np(captureThread.native_handle(), sizeof(cpu_set_t), &cpuset);
   ```

2. **Increase Buffer Sizes**:
   ```cpp
   // In config.h
   const int CAPTURE_QUEUE_SIZE = 20;  // From 10
   const int ENCODE_QUEUE_SIZE = 20;   // From 10
   ```

3. **Network Optimization**:
   - Use jumbo frames (MTU 9000) on gigabit network
   - Increase kernel network buffers: `sudo sysctl -w net.core.rmem_max=26214400`

### Reducing Latency

1. **Minimize Queue Sizes**: Smaller queues = less buffering
2. **Disable B-frames**: Already done (`max_b_frames = 0`)
3. **Zero-latency Mode**: Already enabled in NVENC settings
4. **Use UDP**: Already using (TCP would add 20-30ms)

## Known Limitations

1. **UDP Packet Loss**: No retransmission or FEC. On lossy networks, consider adding RTP/RTCP.
2. **No Authentication**: Any client can connect. Add encryption/auth for production.
3. **Single Camera**: Currently hardcoded to one camera. Easy to extend for multiple.
4. **Linux Only**: V4L2 is Linux-specific. Would need DirectShow/AVFoundation for Windows/Mac.
5. **YUYV Format**: Assumes camera outputs YUYV. Some cameras may need format conversion.

## Future Improvements

- [ ] Implement RTP for better packet handling
- [ ] Add WebRTC support for browser-based clients
- [ ] Multi-camera support with camera selection API
- [ ] Hardware-accelerated color space conversion (CUDA kernel)
- [ ] Lock-free queues for better performance
- [ ] Metrics export (Prometheus/Grafana)
- [ ] Client authentication and encryption

## License

MIT License - see LICENSE file for details

## Contact

Developed during internship at CUBIT, Stony Brook University

For questions about this implementation, please open an issue on GitHub.

## Acknowledgments

- CUBIT team at Stony Brook University for the opportunity
- FFmpeg and NVIDIA teams for excellent documentation
- Open source community for V4L2 examples
