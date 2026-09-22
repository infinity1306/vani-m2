#pragma once

#include <vector>
#include <atomic>
#include <mutex>
#include <optional>
#include <span>
#include <cstring>
#include <algorithm>

namespace vani::audio {

/**
 * @brief High-performance bounded audio ring buffer.
 * Supports producer/consumer isolation, overflow tracking, and sample timestamping.
 */
template <typename T = float>
class AudioRingBuffer {
public:
    explicit AudioRingBuffer(size_t capacity_samples)
        : capacity_(capacity_samples), buffer_(capacity_samples, T{0}) {}

    ~AudioRingBuffer() = default;

    AudioRingBuffer(const AudioRingBuffer&) = delete;
    AudioRingBuffer& operator=(const AudioRingBuffer&) = delete;

    size_t write(std::span<const T> samples) {
        if (samples.empty()) return 0;

        std::unique_lock<std::mutex> lock(mutex_);
        size_t available = capacity_ - size_;
        size_t to_write = std::min(samples.size(), available);

        if (to_write < samples.size()) {
            overflow_count_.fetch_add(samples.size() - to_write, std::memory_order_relaxed);
        }

        if (to_write == 0) return 0;

        for (size_t i = 0; i < to_write; ++i) {
            buffer_[write_pos_] = samples[i];
            write_pos_ = (write_pos_ + 1) % capacity_;
        }

        size_ += to_write;
        total_samples_written_.fetch_add(to_write, std::memory_order_relaxed);
        return to_write;
    }

    size_t read(std::span<T> out_buffer) {
        if (out_buffer.empty()) return 0;

        std::unique_lock<std::mutex> lock(mutex_);
        if (size_ == 0) return 0;

        size_t to_read = std::min(out_buffer.size(), size_);

        for (size_t i = 0; i < to_read; ++i) {
            out_buffer[i] = buffer_[read_pos_];
            read_pos_ = (read_pos_ + 1) % capacity_;
        }

        size_ -= to_read;
        total_samples_read_.fetch_add(to_read, std::memory_order_relaxed);
        return to_read;
    }

    void clear() {
        std::unique_lock<std::mutex> lock(mutex_);
        read_pos_ = 0;
        write_pos_ = 0;
        size_ = 0;
    }

    [[nodiscard]] size_t size() const noexcept {
        std::unique_lock<std::mutex> lock(mutex_);
        return size_;
    }

    [[nodiscard]] size_t capacity() const noexcept {
        return capacity_;
    }

    [[nodiscard]] bool empty() const noexcept {
        return size() == 0;
    }

    [[nodiscard]] uint64_t overflow_count() const noexcept {
        return overflow_count_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] uint64_t total_written() const noexcept {
        return total_samples_written_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] uint64_t total_read() const noexcept {
        return total_samples_read_.load(std::memory_order_relaxed);
    }

private:
    const size_t capacity_;
    std::vector<T> buffer_;
    mutable std::mutex mutex_;
    size_t read_pos_{0};
    size_t write_pos_{0};
    size_t size_{0};

    std::atomic<uint64_t> overflow_count_{0};
    std::atomic<uint64_t> total_samples_written_{0};
    std::atomic<uint64_t> total_samples_read_{0};
};

} // namespace vani::audio
