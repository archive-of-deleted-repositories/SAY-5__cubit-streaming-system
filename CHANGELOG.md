# Changelog

All notable changes to the CUBIT Streaming System will be documented in this file.

## [1.0.2] - 2024-06-22

### Added
- **Comprehensive configuration validation** in `src/utils/config_loader.h`
  - Bounds checking for all numeric parameters (width, height, fps, bitrate, ports, queue sizes)
  - Prevents critical crashes from invalid values (e.g., fps=0 causing division by zero)
  - H.264 compatibility validation (dimensions must be even)
  - Logical constraint validation (min_bitrate ≤ max_bitrate)
  - Encoder preset enumeration validation
  - Path validation for camera device files
  - Detailed error messages showing valid ranges and actual values
- **KNOWN_ISSUES.md** documentation
  - Detailed analysis of current limitations
  - Workarounds and recommended solutions
  - Testing procedures for hardware deployment
  - Future enhancement roadmap
- **Enhanced README.md** with expanded Known Limitations section
  - Documents custom fragmentation protocol requirements
  - Clarifies hardware testing status
  - Provides context for production deployment

### Changed
- Configuration loader now validates all inputs before applying
- Invalid configuration values are rejected with detailed error messages
- Default values used when validation fails (with warnings logged)

### Technical Details
- Added `ConfigLoader::Limits` structure with constexpr bounds
  - Camera: 320-3840 × 240-2160, 1-240 fps
  - Bitrate: 100 kbps - 50 Mbps
  - Ports: 1024-65535
  - Queue sizes: 2-100
- Enhanced error handling with try-catch for parsing exceptions
- Configuration summary printed after successful validation

### Security
- Prevents potential crashes from malformed configuration files
- Validates file paths are absolute (prevents relative path exploits)
- Bounds checking prevents integer overflow scenarios

## [1.0.1] - 2024-06-20

### Fixed
- **CRITICAL**: Added client registration mechanism (control thread)
  - Clients can now properly register by sending HELLO packets
  - Server listens for registration packets on same UDP port
  - This was a major oversight - system wouldn't work without it!
- **CRITICAL**: Implemented packet fragmentation for large frames
  - Keyframes now properly split into MTU-sized packets
  - Prevents corruption of large frames (>1400 bytes)
  - Added PacketHeader structure for fragment reassembly

### Added
- Configuration file support (streaming.conf)
  - Simple KEY=VALUE format for runtime configuration
  - No need to recompile for setting changes
  - Command line argument: `./streaming_service config.conf`
- Control thread for handling client connections
- Automatic client cleanup (30 second timeout)

### Changed
- Updated client_example.sh to send HELLO packet before connecting
- Changed Config constants to inline variables for runtime modification
- Improved network error handling

## [1.0.0] - 2024-06-15

### Added
- Initial release of low-latency video streaming system
- V4L2 camera capture module with ring buffer
- FFmpeg-based video encoder with NVENC hardware acceleration
- Automatic fallback to CPU encoding (libx264) when NVENC unavailable
- UDP network streaming to multiple clients
- Multi-threaded pipeline architecture
  - Dedicated capture, encoding, network, and adaptation threads
  - Thread-safe queues for inter-thread communication
- GPU monitoring using NVIDIA Management Library (NVML)
- Adaptive bitrate control based on GPU utilization
- Performance tracking and metrics
  - FPS monitoring
  - Encode time tracking
  - Frame drop detection
- Docker support with GPU passthrough
- Docker Compose configuration for easy deployment
- Comprehensive documentation and build scripts

### Technical Details
- Resolution: 1920x1080 @ 60fps
- Bitrate range: 2-10 Mbps (adaptive)
- Target latency: 50-70ms end-to-end
- H.264 codec with zero-latency settings
- No B-frames for minimal latency
- GOP size: 60 frames (1 keyframe/second)

## [0.3.0] - 2024-05-28

### Added
- Adaptive bitrate controller with hysteresis
- GPU utilization monitoring

### Fixed
- Bitrate oscillation issue by adding hysteresis (3 consecutive readings)
- Memory leak in adaptation controller

### Changed
- Encoder preset from "ultrafast" to "fast" for better quality
- Default bitrate increased from 3 Mbps to 5 Mbps

## [0.2.0] - 2024-05-10

### Added
- UDP network streaming module
- Client connection tracking
- Performance tracker with sliding window averaging

### Fixed
- Deep copy bug in camera capture (frames were being corrupted)
  - Camera reuses internal buffers, needed to deep copy immediately
  - This was a critical bug that took 2 days to debug!

### Changed
- Increased UDP send buffer to 1 MB for better throughput

## [0.1.0] - 2024-04-25

### Added
- Initial project structure
- V4L2 camera capture implementation
- FFmpeg encoder wrapper
- Basic multi-threaded architecture
- CMake build system

### Known Issues
- Frame corruption under load (fixed in 0.2.0)
- Quality issues with "ultrafast" preset (fixed in 0.3.0)
