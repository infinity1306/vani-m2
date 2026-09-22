#pragma once

#include <span>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <chrono>

namespace vani::audio::vad {

struct AdaptiveVadConfig {
    float initial_noise_floor{0.00025f};
    float min_noise_floor{0.00010f};
    float max_noise_floor{0.0050f};

    // Multipliers relative to estimated ambient noise floor
    float onset_multiplier{1.75f};   // frame_rms > noise_floor * 1.75 to trigger speech onset
    float offset_multiplier{1.30f};  // frame_rms < noise_floor * 1.30 to begin silence release

    // Conservative absolute floors to guarantee stability in quiet background noise
    float min_onset_threshold{0.00040f};  // Floor for speech detection on this laptop mic array
    float min_offset_threshold{0.00028f}; // Floor for silence release

    // Noise floor adaptation rates (EMA)
    // Scaled for ~30-50 frames/sec (time constant ~2-5 seconds)
    float alpha_noise_down{0.015f};  // Tracking downward (~70 frames = ~2s)
    float alpha_noise_up{0.003f};    // Very slow tracking upward (~300 frames = ~10s)

    // Temporal debouncing / hysteresis against flapping
    size_t attack_frames{3};         // 3 consecutive frames (~60-90ms) above onset threshold to declare speech
    size_t release_frames{15};       // 15 consecutive frames (~300-450ms) below offset threshold to declare silence (hangover)
    size_t warmup_frames{20};        // Warmup frames on stream start to discard initial hardware DC pops (~400-600ms)
};

struct AdaptiveVadFrameResult {
    float frame_rms{0.0f};
    float noise_floor_rms{0.0f};
    float onset_threshold{0.0f};
    float offset_threshold{0.0f};
    bool is_speech{false};
    bool speech_onset{false};        // True only on the rising edge
    bool speech_offset{false};       // True only on the falling edge
};

class AdaptiveVadDetector {
public:
    explicit AdaptiveVadDetector(const AdaptiveVadConfig& config = {})
        : config_(config),
          noise_floor_rms_(config.initial_noise_floor) {}

    void reset() {
        noise_floor_rms_ = config_.initial_noise_floor;
        current_frame_rms_ = 0.0f;
        consecutive_speech_frames_ = 0;
        consecutive_silence_frames_ = 0;
        is_speech_active_ = false;
        speech_frame_count_ = 0;
        silence_frame_count_ = 0;
        speech_start_timestamp_ns_ = 0;
        speech_end_timestamp_ns_ = 0;
        total_frames_processed_ = 0;
    }

    void reset_session() {
        consecutive_speech_frames_ = 0;
        consecutive_silence_frames_ = 0;
        is_speech_active_ = false;
        speech_frame_count_ = 0;
        silence_frame_count_ = 0;
        speech_start_timestamp_ns_ = 0;
        speech_end_timestamp_ns_ = 0;
    }

    AdaptiveVadFrameResult process_frame(std::span<const float> frame, bool is_idle = false) {
        (void)is_idle;
        if (frame.empty()) {
            return {
                .frame_rms = 0.0f,
                .noise_floor_rms = noise_floor_rms_,
                .onset_threshold = adaptive_onset_threshold(),
                .offset_threshold = adaptive_offset_threshold(),
                .is_speech = is_speech_active_,
                .speech_onset = false,
                .speech_offset = false
            };
        }

        total_frames_processed_++;

        // 1. Calculate frame RMS energy
        double sum_sq = 0.0;
        for (float s : frame) {
            sum_sq += static_cast<double>(s * s);
        }
        float frame_rms = static_cast<float>(std::sqrt(sum_sq / static_cast<double>(frame.size())));
        current_frame_rms_ = frame_rms;

        uint64_t now_ns = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count()
        );

        float onset_thresh = adaptive_onset_threshold();
        float offset_thresh = adaptive_offset_threshold();

        bool speech_onset = false;
        bool speech_offset = false;

        // Skip speech detection during initial hardware warmup
        if (total_frames_processed_ <= config_.warmup_frames) {
            update_noise_floor(frame_rms);
            silence_frame_count_++;
            return AdaptiveVadFrameResult{
                .frame_rms = frame_rms,
                .noise_floor_rms = noise_floor_rms_,
                .onset_threshold = onset_thresh,
                .offset_threshold = offset_thresh,
                .is_speech = false,
                .speech_onset = false,
                .speech_offset = false
            };
        }

        // 2. Speech state logic with hysteresis and temporal debouncing
        if (!is_speech_active_) {
            // Idle / Silence: looking for onset
            if (frame_rms >= onset_thresh) {
                consecutive_speech_frames_++;
                consecutive_silence_frames_ = 0;
                if (consecutive_speech_frames_ >= config_.attack_frames) {
                    is_speech_active_ = true;
                    speech_onset = true;
                    if (speech_start_timestamp_ns_ == 0) {
                        speech_start_timestamp_ns_ = now_ns;
                    }
                }
            } else {
                consecutive_speech_frames_ = 0;
                consecutive_silence_frames_++;
                
                // Track ambient noise floor during confirmed silence
                update_noise_floor(frame_rms);
            }
        } else {
            // Speech is active: looking for offset (silence hangover)
            if (frame_rms >= offset_thresh) {
                consecutive_speech_frames_++;
                consecutive_silence_frames_ = 0;
            } else {
                consecutive_silence_frames_++;
                if (consecutive_silence_frames_ >= config_.release_frames) {
                    is_speech_active_ = false;
                    speech_offset = true;
                    speech_end_timestamp_ns_ = now_ns;
                    consecutive_speech_frames_ = 0;
                }
            }
            // During active speech, do NOT adapt noise floor upward
        }

        // Count frames
        if (is_speech_active_) {
            speech_frame_count_++;
        } else {
            silence_frame_count_++;
        }

        return AdaptiveVadFrameResult{
            .frame_rms = frame_rms,
            .noise_floor_rms = noise_floor_rms_,
            .onset_threshold = onset_thresh,
            .offset_threshold = offset_thresh,
            .is_speech = is_speech_active_,
            .speech_onset = speech_onset,
            .speech_offset = speech_offset
        };
    }

    [[nodiscard]] float noise_floor_rms() const noexcept { return noise_floor_rms_; }
    [[nodiscard]] float adaptive_vad_threshold() const noexcept { return adaptive_onset_threshold(); }
    [[nodiscard]] float adaptive_onset_threshold() const noexcept {
        return std::max(noise_floor_rms_ * config_.onset_multiplier, config_.min_onset_threshold);
    }
    [[nodiscard]] float adaptive_offset_threshold() const noexcept {
        return std::max(noise_floor_rms_ * config_.offset_multiplier, config_.min_offset_threshold);
    }
    [[nodiscard]] float current_frame_rms() const noexcept { return current_frame_rms_; }
    [[nodiscard]] uint64_t speech_frame_count() const noexcept { return speech_frame_count_; }
    [[nodiscard]] uint64_t silence_frame_count() const noexcept { return silence_frame_count_; }
    [[nodiscard]] uint64_t speech_start_timestamp_ns() const noexcept { return speech_start_timestamp_ns_; }
    [[nodiscard]] uint64_t speech_end_timestamp_ns() const noexcept { return speech_end_timestamp_ns_; }
    [[nodiscard]] bool is_speech_active() const noexcept { return is_speech_active_; }
    [[nodiscard]] const AdaptiveVadConfig& config() const noexcept { return config_; }

private:
    void update_noise_floor(float frame_rms) {
        // Only adapt when frame is below the speech onset threshold
        float onset_thresh = adaptive_onset_threshold();
        if (frame_rms >= onset_thresh) {
            return;
        }

        if (frame_rms < noise_floor_rms_) {
            // Track downward with alpha_noise_down
            noise_floor_rms_ += config_.alpha_noise_down * (frame_rms - noise_floor_rms_);
        } else {
            // Track upward very slowly with alpha_noise_up
            noise_floor_rms_ += config_.alpha_noise_up * (frame_rms - noise_floor_rms_);
        }

        // Clamp to configured physical boundaries
        noise_floor_rms_ = std::clamp(noise_floor_rms_, config_.min_noise_floor, config_.max_noise_floor);
    }

    AdaptiveVadConfig config_;
    float noise_floor_rms_{0.00030f};
    float current_frame_rms_{0.0f};
    size_t consecutive_speech_frames_{0};
    size_t consecutive_silence_frames_{0};
    bool is_speech_active_{false};

    uint64_t speech_frame_count_{0};
    uint64_t silence_frame_count_{0};
    uint64_t speech_start_timestamp_ns_{0};
    uint64_t speech_end_timestamp_ns_{0};
    uint64_t total_frames_processed_{0};
};

} // namespace vani::audio::vad
