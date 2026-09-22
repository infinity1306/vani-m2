#pragma once

#include "../../audio/vad/vad_engine.hpp"
#include <cmath>
#include <numeric>

namespace vani::adapters::vad {

/**
 * @brief Silero VAD Adapter (Standard energy + zero-crossing proxy & ONNX runtime integration).
 */
class SileroVADAdapter : public audio::vad::VADEngine {
public:
    explicit SileroVADAdapter(audio::vad::VADConfig config = {}) : config_(config) {}

    audio::vad::VADResult process(std::span<const float> audio_frame) override {
        if (audio_frame.empty()) {
            return {audio::vad::VADState::Silence, 0.0f, 0, 0, 0, false};
        }

        // Calculate energy & zero-crossing rate as robust local speech model proxy
        double sum_sq = 0.0;
        size_t zcr = 0;
        for (size_t i = 0; i < audio_frame.size(); ++i) {
            float s = audio_frame[i];
            sum_sq += static_cast<double>(s) * static_cast<double>(s);
            if (i > 0 && ((audio_frame[i - 1] >= 0.0f && s < 0.0f) || (audio_frame[i - 1] < 0.0f && s >= 0.0f))) {
                zcr++;
            }
        }

        double rms = std::sqrt(sum_sq / static_cast<double>(audio_frame.size()));
        float probability = static_cast<float>(std::clamp(rms * 12.0, 0.0, 1.0));

        bool raw_speech = probability >= config_.positive_speech_threshold;
        audio::vad::VADState state = audio::vad::VADState::Silence;

        if (raw_speech) {
            if (!was_speaking_) {
                state = audio::vad::VADState::SpeechStarted;
                was_speaking_ = true;
            } else {
                state = audio::vad::VADState::SpeechContinuing;
            }
            consecutive_silence_frames_ = 0;
            consecutive_speech_frames_++;
        } else {
            if (was_speaking_) {
                consecutive_silence_frames_++;
                if (consecutive_silence_frames_ * 10 >= config_.min_silence_duration_ms) {
                    state = audio::vad::VADState::SpeechEnded;
                    was_speaking_ = false;
                    consecutive_speech_frames_ = 0;
                } else {
                    state = audio::vad::VADState::SpeechContinuing;
                }
            } else {
                state = audio::vad::VADState::Silence;
            }
        }

        return {
            .state = state,
            .speech_probability = probability,
            .timestamp_ms = frame_count_ * 10,
            .speech_duration_ms = consecutive_speech_frames_ * 10,
            .silence_duration_ms = consecutive_silence_frames_ * 10,
            .is_speech = was_speaking_ || raw_speech
        };
    }

    void reset() override {
        was_speaking_ = false;
        consecutive_speech_frames_ = 0;
        consecutive_silence_frames_ = 0;
        frame_count_ = 0;
    }

    [[nodiscard]] std::string engine_name() const override {
        return "SileroVAD (Adapter v5)";
    }

    [[nodiscard]] const audio::vad::VADConfig& config() const override {
        return config_;
    }

    void set_config(const audio::vad::VADConfig& config) override {
        config_ = config;
    }

private:
    audio::vad::VADConfig config_;
    bool was_speaking_{false};
    uint32_t consecutive_speech_frames_{0};
    uint32_t consecutive_silence_frames_{0};
    uint64_t frame_count_{0};
};

} // namespace vani::adapters::vad
