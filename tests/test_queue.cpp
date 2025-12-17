// Simple test for ThreadSafeQueue
// Compile with: g++ -std=c++17 -pthread test_queue.cpp -o test_queue
// Run with: ./test_queue

#include "../src/utils/thread_safe_queue.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>

void testBasicOperations() {
    std::cout << "Testing basic operations..." << std::endl;

    ThreadSafeQueue<int> queue(5);

    // Test push and pop
    queue.push(1);
    queue.push(2);
    queue.push(3);

    auto val1 = queue.pop();
    assert(val1.has_value() && val1.value() == 1);

    auto val2 = queue.pop();
    assert(val2.has_value() && val2.value() == 2);

    auto val3 = queue.pop();
    assert(val3.has_value() && val3.value() == 3);

    std::cout << "✓ Basic operations passed" << std::endl;
}

void testTryPush() {
    std::cout << "Testing tryPush with full queue..." << std::endl;

    ThreadSafeQueue<int> queue(3);

    assert(queue.tryPush(1) == true);
    assert(queue.tryPush(2) == true);
    assert(queue.tryPush(3) == true);
    assert(queue.tryPush(4) == false);  // Should fail, queue full

    std::cout << "✓ tryPush test passed" << std::endl;
}

void testThreadSafety() {
    std::cout << "Testing thread safety..." << std::endl;

    ThreadSafeQueue<int> queue(100);
    const int NUM_ITEMS = 1000;

    // Producer thread
    std::thread producer([&queue]() {
        for (int i = 0; i < NUM_ITEMS; i++) {
            queue.push(i);
        }
    });

    // Consumer thread
    std::thread consumer([&queue]() {
        for (int i = 0; i < NUM_ITEMS; i++) {
            auto val = queue.pop();
            assert(val.has_value());
            assert(val.value() == i);  // Should be in order
        }
    });

    producer.join();
    consumer.join();

    std::cout << "✓ Thread safety test passed" << std::endl;
}

void testStop() {
    std::cout << "Testing stop functionality..." << std::endl;

    ThreadSafeQueue<int> queue(10);

    // Start a thread waiting on pop
    bool threadDone = false;
    std::thread waiter([&queue, &threadDone]() {
        auto val = queue.pop();
        assert(!val.has_value());  // Should return nullopt after stop
        threadDone = true;
    });

    // Give thread time to start waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop the queue
    queue.stop();

    // Thread should wake up and finish
    waiter.join();
    assert(threadDone);

    std::cout << "✓ Stop test passed" << std::endl;
}

int main() {
    std::cout << "==================================" << std::endl;
    std::cout << "ThreadSafeQueue Unit Tests" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << std::endl;

    testBasicOperations();
    testTryPush();
    testThreadSafety();
    testStop();

    std::cout << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "All tests passed! ✓" << std::endl;
    std::cout << "==================================" << std::endl;

    return 0;
}
