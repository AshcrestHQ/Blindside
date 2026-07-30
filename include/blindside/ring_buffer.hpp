#ifndef BLINDSIDE_RING_BUFFER_HPP
#define BLINDSIDE_RING_BUFFER_HPP

#include <array>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <utility>
#include <cstddef>

namespace blindside {

/**
 * @brief Thread-safe zero-allocation circular ring buffer for real-time frame transport.
 * 
 * Optimized for producer-consumer pipelines (e.g. camera capture -> vision worker thread).
 * Storage is pre-allocated on stack/heap initialization, guaranteeing zero runtime dynamic
 * memory allocations during push/pop operations.
 */
template <typename T, std::size_t Capacity>
class RingBuffer {
public:
    static_assert(Capacity > 0, "Capacity must be greater than 0");

    RingBuffer() : head_(0), tail_(0), size_(0), stopped_(false) {}

    ~RingBuffer() = default;

    // Disable copying to enforce clear ownership semantics
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    /**
     * @brief Pushes an item into the buffer. If full, drops the oldest frame.
     * Guaranteed zero heap allocation.
     */
    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopped_) return;

            buffer_[head_] = std::move(item);
            head_ = (head_ + 1) % Capacity;

            if (size_ < Capacity) {
                ++size_;
            } else {
                // Buffer overflow: drop oldest item (move tail forward)
                tail_ = (tail_ + 1) % Capacity;
            }
        }
        cv_.notify_one();
    }

    /**
     * @brief Non-blocking attempt to pop the latest or next item.
     * @return std::optional<T> containing popped item if available.
     */
    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (size_ == 0) {
            return std::nullopt;
        }

        T item = std::move(buffer_[tail_]);
        tail_ = (tail_ + 1) % Capacity;
        --size_;
        return item;
    }

    /**
     * @brief Blocking pop with timeout.
     */
    template <typename Rep, typename Period>
    std::optional<T> wait_pop_for(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!cv_.wait_for(lock, timeout, [this]() { return size_ > 0 || stopped_; })) {
            return std::nullopt;
        }

        if (stopped_ && size_ == 0) {
            return std::nullopt;
        }

        T item = std::move(buffer_[tail_]);
        tail_ = (tail_ + 1) % Capacity;
        --size_;
        return item;
    }

    /**
     * @brief Returns current element count.
     */
    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_;
    }

    /**
     * @brief Clears the buffer contents.
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        head_ = 0;
        tail_ = 0;
        size_ = 0;
    }

    /**
     * @brief Stops the buffer, unblocking any waiting threads.
     */
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        cv_.notify_all();
    }

    bool is_stopped() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stopped_;
    }

private:
    std::array<T, Capacity> buffer_;
    std::size_t head_;
    std::size_t tail_;
    std::size_t size_;
    bool stopped_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace blindside

#endif // BLINDSIDE_RING_BUFFER_HPP
