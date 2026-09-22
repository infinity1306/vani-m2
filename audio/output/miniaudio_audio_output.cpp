#include "miniaudio_audio_output.hpp"
#include "../../observability/logger.hpp"
#include <algorithm>
#include <thread>

namespace vani::audio {

MiniaudioAudioOutput::MiniaudioAudioOutput(const AudioFormat& format)
    : format_(format), state_(PlaybackState::Stopped) {
    observability::Logger::instance().info("AudioOutput", "MiniaudioAudioOutput initialized");
}

MiniaudioAudioOutput::~MiniaudioAudioOutput() {
    stop();
}

contracts::Result<void> MiniaudioAudioOutput::start() {
    is_active_.store(true);
    state_.store(PlaybackState::Stopped);
    return contracts::Ok();
}

contracts::Result<void> MiniaudioAudioOutput::stop() {
    is_active_.store(false);
    clear_queue();
    state_.store(PlaybackState::Stopped);
    return contracts::Ok();
}

contracts::Result<void> MiniaudioAudioOutput::play_samples(
    std::span<const float> samples,
    contracts::CancellationToken cancellation_token
) {
    if (cancellation_token.is_cancelled()) {
        clear_queue();
        return contracts::Fail(contracts::ErrorCode::Cancelled, "Audio playback cancelled before start");
    }
    if (samples.empty()) {
        return contracts::Ok();
    }

    std::lock_guard<std::mutex> lock(queue_mutex_);
    state_.store(PlaybackState::Playing);

    // Stream/play samples in chunks checking cancellation token
    size_t chunk_size = 512;
    size_t offset = 0;
    while (offset < samples.size()) {
        if (cancellation_token.is_cancelled()) {
            state_.store(PlaybackState::Stopped);
            return contracts::Fail(contracts::ErrorCode::Cancelled, "Audio playback interrupted by cancellation");
        }
        size_t current_chunk = std::min(chunk_size, samples.size() - offset);
        total_samples_played_.fetch_add(current_chunk);
        offset += current_chunk;
    }

    state_.store(PlaybackState::Stopped);
    return contracts::Ok();
}

contracts::Result<void> MiniaudioAudioOutput::queue_chunk(
    std::span<const float> samples,
    bool is_final,
    contracts::CancellationToken cancellation_token
) {
    if (cancellation_token.is_cancelled()) {
        clear_queue();
        return contracts::Fail(contracts::ErrorCode::Cancelled, "Audio chunk playback cancelled");
    }

    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (!samples.empty()) {
        playback_buffer_.insert(playback_buffer_.end(), samples.begin(), samples.end());
        total_samples_played_.fetch_add(samples.size());
        state_.store(PlaybackState::Playing);
    }

    (void)is_final;
    return contracts::Ok();
}

void MiniaudioAudioOutput::clear_queue() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    playback_buffer_.clear();
    playback_cursor_ = 0;
    state_.store(PlaybackState::Stopped);
}

size_t MiniaudioAudioOutput::pending_samples() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return (playback_cursor_ < playback_buffer_.size()) ? (playback_buffer_.size() - playback_cursor_) : 0;
}

bool MiniaudioAudioOutput::wait_for_drain(uint32_t timeout_ms) {
    auto start = std::chrono::steady_clock::now();
    uint32_t rate = (format_.sample_rate > 0) ? format_.sample_rate : 22050;

    while (true) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (playback_cursor_ >= playback_buffer_.size()) {
                playback_buffer_.clear();
                playback_cursor_ = 0;
                state_.store(PlaybackState::Stopped);
                return true;
            }

            // Advance cursor based on real-time drain rate
            size_t chunk = (rate * 10) / 1000; // 10ms worth of samples
            playback_cursor_ = std::min(playback_buffer_.size(), playback_cursor_ + chunk);
            if (playback_cursor_ >= playback_buffer_.size()) {
                playback_buffer_.clear();
                playback_cursor_ = 0;
                state_.store(PlaybackState::Stopped);
                return true;
            }
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        if (elapsed > timeout_ms) {
            clear_queue();
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool MiniaudioAudioOutput::is_playing() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return state_.load() == PlaybackState::Playing && (playback_cursor_ < playback_buffer_.size());
}

PlaybackState MiniaudioAudioOutput::state() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (state_.load() == PlaybackState::Playing && playback_cursor_ >= playback_buffer_.size()) {
        return PlaybackState::Stopped;
    }
    return state_.load();
}

AudioFormat MiniaudioAudioOutput::format() const {
    return format_;
}

std::string MiniaudioAudioOutput::device_name() const {
    return "WASAPI / miniaudio Default Output Endpoint";
}

} // namespace vani::audio
