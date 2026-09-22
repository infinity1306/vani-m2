#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace vani::audio {

enum class SampleFormat : uint8_t {
    Float32,
    Int16,
    Int32
};

struct AudioFormat {
    uint32_t sample_rate{16000}; // 16kHz speech standard
    uint8_t channels{1};         // Mono standard for STT/VAD
    SampleFormat format{SampleFormat::Float32};
    uint32_t frame_size_samples{160}; // 10ms at 16kHz
    std::string device_id{"default_input"};

    [[nodiscard]] constexpr size_t bytes_per_sample() const noexcept {
        switch (format) {
            case SampleFormat::Float32: return 4;
            case SampleFormat::Int16:   return 2;
            case SampleFormat::Int32:   return 4;
        }
        return 4;
    }

    [[nodiscard]] constexpr size_t frame_size_bytes() const noexcept {
        return frame_size_samples * channels * bytes_per_sample();
    }

    [[nodiscard]] constexpr double frame_duration_ms() const noexcept {
        if (sample_rate == 0) return 0.0;
        return (static_cast<double>(frame_size_samples) / static_rate()) * 1000.0;
    }

private:
    [[nodiscard]] constexpr double static_rate() const noexcept {
        return static_cast<double>(sample_rate);
    }
};

struct AudioFrame {
    AudioFormat format;
    uint64_t timestamp_ns{0};
    uint64_t sequence_number{0};
};

} // namespace vani::audio
