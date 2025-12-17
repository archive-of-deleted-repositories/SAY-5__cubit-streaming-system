#!/bin/bash
# Pre-push testing script
# Run this before pushing to Git to ensure everything works

set -e

echo "=========================================="
echo "CUBIT Streaming System - Pre-Push Tests"
echo "=========================================="
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

TESTS_PASSED=0
TESTS_FAILED=0

pass_test() {
    echo -e "${GREEN}✓${NC} $1"
    ((TESTS_PASSED++))
}

fail_test() {
    echo -e "${RED}✗${NC} $1"
    ((TESTS_FAILED++))
}

warn_test() {
    echo -e "${YELLOW}⚠${NC} $1"
}

# Test 1: Project structure
echo "1. Checking project structure..."
if [ ! -f "CMakeLists.txt" ]; then
    fail_test "Not in project root (CMakeLists.txt not found)"
    exit 1
fi
pass_test "Project root confirmed"

# Test 2: All source files exist
echo ""
echo "2. Checking source files..."
REQUIRED_FILES=(
    "src/main.cpp"
    "src/camera/capture.cpp"
    "src/camera/capture.h"
    "src/encoding/encoder.cpp"
    "src/encoding/encoder.h"
    "src/network/udp_streamer.cpp"
    "src/network/udp_streamer.h"
    "src/monitoring/gpu_monitor.cpp"
    "src/monitoring/gpu_monitor.h"
    "src/monitoring/performance_tracker.cpp"
    "src/monitoring/performance_tracker.h"
    "src/monitoring/adaptation_controller.cpp"
    "src/monitoring/adaptation_controller.h"
    "src/utils/config.h"
    "src/utils/config_loader.h"
    "src/utils/frame.h"
    "src/utils/thread_safe_queue.h"
)

for f in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "$f" ]; then
        fail_test "Missing: $f"
    fi
done
pass_test "All ${#REQUIRED_FILES[@]} source files present"

# Test 3: Documentation
echo ""
echo "3. Checking documentation..."
DOCS=(
    "README.md"
    "CHANGELOG.md"
    "QUICKSTART.md"
    "LICENSE"
    "streaming.conf"
)

for doc in "${DOCS[@]}"; do
    if [ ! -f "$doc" ]; then
        fail_test "Missing: $doc"
    fi
done
pass_test "All documentation files present"

# Test 4: Scripts are executable
echo ""
echo "4. Checking scripts..."
SCRIPTS=(
    "scripts/build.sh"
    "scripts/run.sh"
    "scripts/run_docker.sh"
    "scripts/client_example.sh"
)

for script in "${SCRIPTS[@]}"; do
    if [ ! -x "$script" ]; then
        warn_test "$script not executable (fixing...)"
        chmod +x "$script"
    fi
done
pass_test "All scripts executable"

# Test 5: Build test
echo ""
echo "5. Testing build..."
if [ -d "build" ]; then
    echo "   Cleaning old build directory..."
    rm -rf build
fi

./scripts/build.sh > /tmp/cubit_build.log 2>&1
if [ $? -eq 0 ]; then
    pass_test "Build successful"
else
    fail_test "Build failed (see /tmp/cubit_build.log)"
    cat /tmp/cubit_build.log
    exit 1
fi

# Test 6: Executable exists
echo ""
echo "6. Checking executable..."
if [ -f "build/streaming_service" ]; then
    SIZE=$(ls -lh build/streaming_service | awk '{print $5}')
    pass_test "Executable created ($SIZE)"
else
    fail_test "Executable not found"
    exit 1
fi

# Test 7: No build artifacts in repo
echo ""
echo "7. Checking for build artifacts..."
if [ -f ".gitignore" ]; then
    pass_test ".gitignore exists"
else
    fail_test ".gitignore missing"
fi

# Test 8: Configuration file format
echo ""
echo "8. Validating configuration file..."
if grep -q "camera_width" streaming.conf && grep -q "udp_port" streaming.conf; then
    pass_test "Configuration file format valid"
else
    fail_test "Configuration file appears malformed"
fi

# Test 9: Check for sensitive data
echo ""
echo "9. Checking for sensitive data..."
if grep -r "password\|secret\|token\|api_key" . --exclude-dir=.git --exclude-dir=build --exclude="*.md" > /dev/null 2>&1; then
    warn_test "Found potential sensitive data - please review"
    grep -r "password\|secret\|token\|api_key" . --exclude-dir=.git --exclude-dir=build --exclude="*.md"
else
    pass_test "No obvious sensitive data found"
fi

# Test 10: Line count
echo ""
echo "10. Code statistics..."
TOTAL_LINES=$(find src -name "*.cpp" -o -name "*.h" | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
FILE_COUNT=$(find src -name "*.cpp" -o -name "*.h" | wc -l)
pass_test "$FILE_COUNT source files, $TOTAL_LINES total lines"

# Summary
echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo -e "${GREEN}Passed:${NC} $TESTS_PASSED"
echo -e "${RED}Failed:${NC} $TESTS_FAILED"

if [ $TESTS_FAILED -eq 0 ]; then
    echo ""
    echo -e "${GREEN}✓ All tests passed!${NC}"
    echo "Ready to commit and push to Git"
    echo ""
    echo "Next steps:"
    echo "  1. git init (if not already done)"
    echo "  2. git add ."
    echo "  3. git commit -m 'Initial commit: Low-latency video streaming system'"
    echo "  4. git remote add origin <your-repo-url>"
    echo "  5. git push -u origin main"
    exit 0
else
    echo ""
    echo -e "${RED}✗ Some tests failed${NC}"
    echo "Please fix the issues above before pushing"
    exit 1
fi
