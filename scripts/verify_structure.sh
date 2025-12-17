#!/bin/bash
# Simple structure verification (no build required)
# Use this on macOS or systems without build dependencies

echo "=========================================="
echo "CUBIT Streaming - Structure Verification"
echo "=========================================="
echo ""

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

PASS=0
FAIL=0

check_file() {
    if [ -f "$1" ]; then
        echo -e "${GREEN}✓${NC} $1"
        ((PASS++))
    else
        echo -e "${RED}✗${NC} $1 (MISSING)"
        ((FAIL++))
    fi
}

echo "Core Source Files:"
check_file "src/main.cpp"
check_file "src/camera/capture.cpp"
check_file "src/camera/capture.h"
check_file "src/encoding/encoder.cpp"
check_file "src/encoding/encoder.h"
check_file "src/network/udp_streamer.cpp"
check_file "src/network/udp_streamer.h"

echo ""
echo "Monitoring Modules:"
check_file "src/monitoring/gpu_monitor.cpp"
check_file "src/monitoring/gpu_monitor.h"
check_file "src/monitoring/performance_tracker.cpp"
check_file "src/monitoring/performance_tracker.h"
check_file "src/monitoring/adaptation_controller.cpp"
check_file "src/monitoring/adaptation_controller.h"

echo ""
echo "Utilities:"
check_file "src/utils/config.h"
check_file "src/utils/config_loader.h"
check_file "src/utils/frame.h"
check_file "src/utils/thread_safe_queue.h"

echo ""
echo "Build System:"
check_file "CMakeLists.txt"
check_file "scripts/build.sh"

echo ""
echo "Docker:"
check_file "docker/Dockerfile"
check_file "docker/docker-compose.yml"

echo ""
echo "Documentation:"
check_file "README.md"
check_file "CHANGELOG.md"
check_file "QUICKSTART.md"
check_file "LICENSE"
check_file "IMPROVEMENTS.md"
check_file "GIT_SETUP.md"

echo ""
echo "Configuration:"
check_file "streaming.conf"
check_file ".gitignore"

echo ""
echo "Scripts:"
check_file "scripts/run.sh"
check_file "scripts/run_docker.sh"
check_file "scripts/client_example.sh"

echo ""
echo "=========================================="
echo "Summary:"
echo -e "${GREEN}Present: $PASS files${NC}"
echo -e "${RED}Missing: $FAIL files${NC}"
echo "=========================================="

if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}✓ All files present - ready for Git!${NC}"
    exit 0
else
    echo -e "${RED}✗ Some files missing${NC}"
    exit 1
fi
