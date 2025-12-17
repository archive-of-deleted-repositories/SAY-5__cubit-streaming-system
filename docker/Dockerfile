# Multi-stage Dockerfile for efficient builds
# Stage 1: Builder
FROM nvidia/cuda:11.8.0-devel-ubuntu20.04 as builder

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    git \
    wget \
    # FFmpeg dependencies
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev \
    # V4L2 for camera
    libv4l-dev \
    v4l-utils \
    && rm -rf /var/lib/apt/lists/*

# Copy source code
WORKDIR /build
COPY . .

# Build the application
# Initially tried building without GPU support but had to add CUDA paths
RUN mkdir build && cd build && \
    cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DNVML_INCLUDE_DIR=/usr/local/cuda/include \
    -DNVML_LIBRARY=/usr/local/cuda/lib64/stubs/libnvidia-ml.so && \
    make -j$(nproc)

# Stage 2: Runtime
FROM nvidia/cuda:11.8.0-runtime-ubuntu20.04

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libavcodec58 \
    libavformat58 \
    libavutil56 \
    libswscale5 \
    libv4l-0 \
    v4l-utils \
    && rm -rf /var/lib/apt/lists/*

# Create app directory
WORKDIR /app

# Copy binary from builder
COPY --from=builder /build/build/streaming_service /app/

# Expose UDP port
EXPOSE 5000/udp

# Default command
CMD ["./streaming_service"]
