#!/bin/bash
# Example client script to receive and display the video stream
# Requires FFmpeg to be installed

set -e

SERVER_IP=${1:-localhost}
PORT=${2:-5000}

echo "======================================"
echo "CUBIT Streaming Client"
echo "======================================"
echo ""
echo "Connecting to: $SERVER_IP:$PORT"
echo ""
echo "Controls:"
echo "  - Q: Quit"
echo "  - F: Fullscreen"
echo ""

# Check if ffplay is available
if ! command -v ffplay &> /dev/null; then
    echo "Error: ffplay not found"
    echo "Install with: sudo apt-get install ffmpeg"
    exit 1
fi

# Send HELLO packet to register with server
echo "Registering with server..."
echo -n "HELLO" | nc -u -w1 $SERVER_IP $PORT

# Give server a moment to register us
sleep 0.5

echo "Starting video playback..."
echo ""

# Receive and play the stream with minimal latency settings
ffplay -fflags nobuffer \
       -flags low_delay \
       -framedrop \
       -strict experimental \
       -vf setpts=0 \
       -sync ext \
       -udp_buffer_size 2097152 \
       udp://$SERVER_IP:$PORT

echo ""
echo "Stream ended"
