#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

// Thread-safe queue for passing frames between threads
// Used mutexes and condition variables since this was most straightforward
// TODO: Implement lock-free queue for better performance if needed
template<typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    size_t maxSize_;
    bool stopped_;

public:
    explicit ThreadSafeQueue(size_t maxSize = 100)
        : maxSize_(maxSize), stopped_(false) {}

    // Push item to queue (blocks if queue is full)
    bool push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (stopped_) {
            return false;
        }

        // Wait if queue is full
        cv_.wait(lock, [this] {
            return queue_.size() < maxSize_ || stopped_;
        });

        if (stopped_) {
            return false;
        }

        queue_.push(std::move(item));
        cv_.notify_one();
        return true;
    }

    // Try to push without blocking - drops frame if queue is full
    // Using this for capture thread to prevent blocking camera
    bool tryPush(T item) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (stopped_ || queue_.size() >= maxSize_) {
            return false;  // Queue full or stopped
        }

        queue_.push(std::move(item));
        cv_.notify_one();
        return true;
    }

    // Pop item from queue (blocks until item available)
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mutex_);

        cv_.wait(lock, [this] {
            return !queue_.empty() || stopped_;
        });

        if (stopped_ && queue_.empty()) {
            return std::nullopt;
        }

        T item = std::move(queue_.front());
        queue_.pop();
        cv_.notify_one();  // Notify pushers
        return item;
    }

    // Try to pop without blocking
    std::optional<T> tryPop() {
        std::unique_lock<std::mutex> lock(mutex_);

        if (queue_.empty()) {
            return std::nullopt;
        }

        T item = std::move(queue_.front());
        queue_.pop();
        cv_.notify_one();
        return item;
    }

    // Get current queue size
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    // Check if queue is empty
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    // Stop the queue and wake all waiting threads
    void stop() {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped_ = true;
        cv_.notify_all();
    }
};

#endif // THREAD_SAFE_QUEUE_H
