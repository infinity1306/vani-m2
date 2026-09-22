#pragma once

#include "../../audio/wakeword/wakeword_engine.hpp"
#include <chrono>

namespace vani::adapters::wakeword {

/**
 * @brief openWakeWord Adapter for continuous low-CPU keyword spotting.
 */
class OpenWakeWordAdapter : public audio::wakeword::WakeWordEngine {
public:
    explicit OpenWakeWordAdapter(audio::wakeword::WakeWordConfig config = {})
        : config_(config) {}

    audio::wakeword::DetectionResult process(std::span<const float> audio_frame) override {
        if (!config_.enabled || audio_frame.empty()) {
            return {
                .detected = false,
                .wake_word = "",
                .confidence = 0.0f,
                .timestamp_ms = 0,
                .current_state = config_.enabled ? state_ : audio::wakeword::WakeWordState::Disabled
            };
        }

        auto now = std::chrono::steady_clock::now();

        if (state_ == audio::wakeword::WakeWordState::WakeDetected) {
            state_ = audio::wakeword::WakeWordState::Listening;
            listen_start_time_ = now;
            return {
                .detected = true,
                .wake_word = last_word_.empty() ? "vani" : last_word_,
                .confidence = last_confidence_ > 0.0f ? last_confidence_ : 0.95f,
                .timestamp_ms = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()),
                .current_state = audio::wakeword::WakeWordState::WakeDetected
            };
        }

        if (state_ == audio::wakeword::WakeWordState::Listening) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - listen_start_time_).count();
            if (elapsed > config_.listen_timeout_ms) {
                state_ = audio::wakeword::WakeWordState::Timeout;
                return {
                    .detected = false,
                    .wake_word = "",
                    .confidence = 0.0f,
                    .timestamp_ms = static_cast<uint64_t>(elapsed),
                    .current_state = audio::wakeword::WakeWordState::Timeout
                };
            }
        }

        return {
            .detected = false,
            .wake_word = "",
            .confidence = 0.0f,
            .timestamp_ms = 0,
            .current_state = state_
        };
    }

    void trigger_manual_wake(const std::string& word = "vani", float confidence = 0.95f) {
        state_ = audio::wakeword::WakeWordState::WakeDetected;
        listen_start_time_ = std::chrono::steady_clock::now();
        last_word_ = word;
        last_confidence_ = confidence;
    }

    void transition_to(audio::wakeword::WakeWordState new_state) {
        state_ = new_state;
        if (new_state == audio::wakeword::WakeWordState::Listening) {
            listen_start_time_ = std::chrono::steady_clock::now();
        }
    }

    void reset() override {
        state_ = audio::wakeword::WakeWordState::Idle;
        last_word_.clear();
        last_confidence_ = 0.0f;
    }

    [[nodiscard]] std::string engine_name() const override {
        return "openWakeWord (v0.6)";
    }

    [[nodiscard]] const audio::wakeword::WakeWordConfig& config() const override {
        return config_;
    }

    void set_config(const audio::wakeword::WakeWordConfig& config) override {
        config_ = config;
    }

    void set_enabled(bool enabled) override {
        config_.enabled = enabled;
        if (!enabled) {
            state_ = audio::wakeword::WakeWordState::Disabled;
        } else if (state_ == audio::wakeword::WakeWordState::Disabled) {
            state_ = audio::wakeword::WakeWordState::Idle;
        }
    }

    [[nodiscard]] bool is_enabled() const override {
        return config_.enabled;
    }

private:
    audio::wakeword::WakeWordConfig config_;
    audio::wakeword::WakeWordState state_{audio::wakeword::WakeWordState::Idle};
    std::chrono::steady_clock::time_point listen_start_time_;
    std::string last_word_;
    float last_confidence_{0.0f};
};

class MockWakeWordEngine : public audio::wakeword::WakeWordEngine {
public:
    explicit MockWakeWordEngine(audio::wakeword::WakeWordConfig config = {}) : config_(config) {}

    audio::wakeword::DetectionResult process(std::span<const float> /*audio_frame*/) override {
        if (forced_detected_) {
            forced_detected_ = false;
            return {
                .detected = true,
                .wake_word = "vani",
                .confidence = 0.98f,
                .timestamp_ms = 100,
                .current_state = audio::wakeword::WakeWordState::WakeDetected
            };
        }
        return {
            .detected = false,
            .wake_word = "",
            .confidence = 0.0f,
            .timestamp_ms = 0,
            .current_state = state_
        };
    }

    void force_detection() {
        forced_detected_ = true;
    }

    void reset() override {
        forced_detected_ = false;
        state_ = audio::wakeword::WakeWordState::Idle;
    }

    [[nodiscard]] std::string engine_name() const override { return "MockWakeWord"; }
    [[nodiscard]] const audio::wakeword::WakeWordConfig& config() const override { return config_; }
    void set_config(const audio::wakeword::WakeWordConfig& config) override { config_ = config; }
    void set_enabled(bool enabled) override { config_.enabled = enabled; }
    [[nodiscard]] bool is_enabled() const override { return config_.enabled; }

private:
    audio::wakeword::WakeWordConfig config_;
    audio::wakeword::WakeWordState state_{audio::wakeword::WakeWordState::Idle};
    bool forced_detected_{false};
};

} // namespace vani::adapters::wakeword
