#!/bin/bash
# Quick test script for queue implementation

set -e

echo "======================================"
echo "Running Unit Tests"
echo "======================================"
echo ""

# Compile test
echo "Compiling test_queue..."
g++ -std=c++17 -pthread tests/test_queue.cpp -o tests/test_queue

# Run test
echo "Running test_queue..."
./tests/test_queue

echo ""
echo "Tests completed successfully!"
