#pragma once

#include "../audio_input.hpp"
#include "../ring_buffer.hpp"
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

// Forward declaration of miniaudio struct
struct ma_device;

namespace vani::audio {

/**
 * @brief Real Windows & cross-platform microphone capture backend using miniaudio (WASAPI on Windows).
 * Provides continuous background audio capture, frame-based delivery, and lock-protected thread safety.
 */
class MiniaudioAudioInput : public AudioInput {
public:
    explicit MiniaudioAudioInput(
        AudioFormat format = {.sample_rate = 16000, .channels = 1, .format = SampleFormat::Float32},
        size_t ring_buffer_capacity = 64000
    );
    ~MiniaudioAudioInput() override;

    contracts::Result<void> start() override;
    contracts::Result<void> stop() override;
    contracts::Result<void> set_callback(AudioCallback callback) override;

    [[nodiscard]] AudioFormat format() const override;
    [[nodiscard]] bool is_capturing() const override;
    [[nodiscard]] std::string device_name() const override;

    /**
     * @brief Access the thread-safe underlying ring buffer filled by microphone frames.
     */
    AudioRingBuffer<float>& ring_buffer() noexcept { return ring_buffer_; }
    const AudioRingBuffer<float>& ring_buffer() const noexcept { return ring_buffer_; }

    // Internal callback invoked from the real-time audio thread
    void on_audio_data(const float* samples, size_t frame_count);

private:
    AudioFormat format_;
    AudioRingBuffer<float> ring_buffer_;
    AudioCallback user_callback_;
    std::string device_name_{"Default System Microphone"};

    std::atomic<bool> is_capturing_{false};
    std::atomic<uint64_t> total_frames_captured_{0};
    mutable std::mutex callback_mutex_;

    // Opaque storage for ma_device to avoid leaking miniaudio types to callers
    void* device_handle_{nullptr};
};

using MiniaudioAudioInputPtr = std::shared_ptr<MiniaudioAudioInput>;

} // namespace vani::audio
