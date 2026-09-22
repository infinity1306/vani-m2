#pragma once

#include "../audio_format.hpp"
#include "../audio_device.hpp"
#include "../../contracts/common/result.hpp"
#include "../../contracts/common/cancellation_token.hpp"
#include <span>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <atomic>
#include <functional>

namespace vani::audio {

enum class PlaybackState : uint8_t {
    Stopped,
    Playing,
    Paused,
    Error
};

class AudioOutput {
public:
    virtual ~AudioOutput() = default;

    virtual contracts::Result<void> start() = 0;
    virtual contracts::Result<void> stop() = 0;
    virtual contracts::Result<void> play_samples(
        std::span<const float> samples,
        contracts::CancellationToken cancellation_token = contracts::CancellationToken::none()
    ) = 0;

    virtual contracts::Result<void> queue_chunk(
        std::span<const float> samples,
        bool is_final,
        contracts::CancellationToken cancellation_token = contracts::CancellationToken::none()
    ) = 0;

    virtual void clear_queue() = 0;
    [[nodiscard]] virtual bool is_playing() const = 0;
    [[nodiscard]] virtual PlaybackState state() const = 0;
    [[nodiscard]] virtual AudioFormat format() const = 0;
    [[nodiscard]] virtual std::string device_name() const = 0;
};

using AudioOutputPtr = std::shared_ptr<AudioOutput>;

class MiniaudioAudioOutput : public AudioOutput {
public:
    explicit MiniaudioAudioOutput(const AudioFormat& format = AudioFormat{22050, 1, SampleFormat::Float32, 512, "default_output"});
    ~MiniaudioAudioOutput() override;

    contracts::Result<void> start() override;
    contracts::Result<void> stop() override;

    contracts::Result<void> play_samples(
        std::span<const float> samples,
        contracts::CancellationToken cancellation_token = contracts::CancellationToken::none()
    ) override;

    contracts::Result<void> queue_chunk(
        std::span<const float> samples,
        bool is_final,
        contracts::CancellationToken cancellation_token = contracts::CancellationToken::none()
    ) override;

    void clear_queue() override;
    [[nodiscard]] bool is_playing() const override;
    [[nodiscard]] PlaybackState state() const override;
    [[nodiscard]] AudioFormat format() const override;
    [[nodiscard]] std::string device_name() const override;

    [[nodiscard]] size_t total_samples_played() const { return total_samples_played_.load(); }
    [[nodiscard]] size_t pending_samples() const;
    bool wait_for_drain(uint32_t timeout_ms = 10000);

private:
    AudioFormat format_;
    std::atomic<PlaybackState> state_{PlaybackState::Stopped};
    std::atomic<bool> is_active_{false};
    std::atomic<size_t> total_samples_played_{0};

    mutable std::mutex queue_mutex_;
    std::vector<float> playback_buffer_;
    size_t playback_cursor_{0};
};

} // namespace vani::audio
