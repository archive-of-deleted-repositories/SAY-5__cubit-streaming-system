# Quick Start Guide

Get the CUBIT Streaming System up and running in 5 minutes.

## Prerequisites

- Ubuntu 20.04 or later (or compatible Linux distribution)
- USB camera or V4L2-compatible video device
- (Optional) NVIDIA GPU with NVENC support for hardware encoding

## 1. Install Dependencies

```bash
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
    v4l-utils \
    ffmpeg
```

## 2. Verify Camera

Check that your camera is detected:

```bash
v4l2-ctl --list-devices
ls -la /dev/video*
```

You should see `/dev/video0` (or similar). If not, plug in your camera and check again.

## 3. Build

```bash
cd cubit-streaming-system
./scripts/build.sh
```

This will:
- Configure the project with CMake
- Compile all source files
- Create the `streaming_service` executable in `build/`

Expected output:
```
======================================
Build completed successfully!
======================================
```

## 4. Run the Server

```bash
./scripts/run.sh
```

You should see:
```
======================================
CUBIT Video Streaming System
Low-Latency Biomedical Microscope Streaming
======================================

[Main] Initializing components...
[Camera] Initializing camera: /dev/video0
[Encoder] Successfully initialized NVENC encoder
[Network] UDP streamer initialized successfully
[Main] Starting streaming pipeline...
```

Press `Ctrl+C` to stop.

## 5. View the Stream (on client machine)

On the same machine or a different computer on the network:

```bash
# Using the provided client script
./scripts/client_example.sh SERVER_IP

# Or use ffplay directly
ffplay -fflags nobuffer -flags low_delay udp://SERVER_IP:5000

# Or use VLC
vlc udp://@:5000
```

Replace `SERVER_IP` with the IP address of the server (use `localhost` if on same machine).

## Troubleshooting

### "Cannot open device /dev/video0"

Your camera might be at a different device path. Check with:
```bash
v4l2-ctl --list-devices
```

Then update `CAMERA_DEVICE` in `src/utils/config.h` and rebuild.

### "NVENC not available"

NVENC requires an NVIDIA GPU. The system will automatically fall back to CPU encoding (libx264), which works fine but is slower. You'll see:
```
[Encoder] NVENC not available, falling back to CPU encoding
[Encoder] Successfully initialized CPU encoder
```

### No video on client

1. Check firewall allows UDP port 5000
2. Verify server is running: `ps aux | grep streaming_service`
3. Check server logs for errors
4. Try localhost first to rule out network issues

## Docker Quick Start

If you prefer Docker:

```bash
# Build and run
./scripts/run_docker.sh

# Or use docker-compose
cd docker
docker-compose up --build
```

**Note**: Docker requires nvidia-container-toolkit for GPU support. See README.md for installation instructions.

## What's Next?

- Read [README.md](README.md) for detailed documentation
- Check [CHANGELOG.md](CHANGELOG.md) for version history
- Modify `src/utils/config.h` to customize settings
- Explore the architecture in `src/` directory

## Common Configurations

### Change Resolution

Edit `src/utils/config.h`:
```cpp
const int CAMERA_WIDTH = 1280;   // Instead of 1920
const int CAMERA_HEIGHT = 720;   // Instead of 1080
```

### Change Port

Edit `src/utils/config.h`:
```cpp
const int UDP_PORT = 8000;  // Instead of 5000
```

### Adjust Bitrate

Edit `src/utils/config.h`:
```cpp
const int INITIAL_BITRATE = 8000000;  // 8 Mbps instead of 5 Mbps
```

After any config change, rebuild with `./scripts/build.sh`.

## Performance Check

After running for ~30 seconds, you should see stats like:

```
[Main] === Stats (uptime: 30s) ===
  Frames captured: 1800
  Frames encoded: 1800
  Current FPS: 60.0
  Avg encode time: 8.5ms
  Drop rate: 0.0%
```

If you see dropped frames or FPS below 60, check the troubleshooting section in README.md.

---

**Need help?** Check the full [README.md](README.md) or open an issue on GitHub.
