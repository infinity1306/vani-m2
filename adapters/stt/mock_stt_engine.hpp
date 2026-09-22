#pragma once

#include "../../contracts/providers/stt_engine.hpp"

namespace vani::adapters::stt {

/**
 * @brief Mock STT engine for deterministic unit & integration tests.
 */
class MockSTTEngine : public contracts::STTEngine {
public:
    explicit MockSTTEngine(std::string default_text = "mera react wala project run karde")
        : canned_text_(std::move(default_text)) {}

    contracts::Result<void> start_stream(
        const contracts::STTConfig& config,
        contracts::TranscriptCallback callback
    ) override {
        config_ = config;
        callback_ = std::move(callback);
        is_streaming_ = true;
        return contracts::Ok();
    }

    contracts::Result<void> push_audio(std::span<const float> /*samples*/) override {
        if (!is_streaming_) return contracts::Ok();
        if (callback_ && config_.enable_interim_results) {
            contracts::STTTranscript partial{
                .text = canned_text_.substr(0, canned_text_.size() / 2),
                .is_final = false,
                .confidence = 0.85f,
                .detected_language = "hinglish",
                .start_time_ms = 0,
                .end_time_ms = 100,
                .sequence_number = 1,
                .engine_name = engine_name()
            };
            callback_(partial);
        }
        return contracts::Ok();
    }

    contracts::Result<void> stop_stream() override {
        if (!is_streaming_) return contracts::Ok();
        is_streaming_ = false;
        if (callback_) {
            contracts::STTTranscript final_t{
                .text = canned_text_,
                .is_final = true,
                .confidence = 0.98f,
                .detected_language = "hinglish",
                .start_time_ms = 0,
                .end_time_ms = 250,
                .sequence_number = 2,
                .engine_name = engine_name()
            };
            callback_(final_t);
        }
        return contracts::Ok();
    }

    void set_canned_text(const std::string& text) { canned_text_ = text; }
    void set_healthy(bool healthy) { is_healthy_ = healthy; }

    [[nodiscard]] std::string engine_name() const override { return "MockSTT"; }
    [[nodiscard]] contracts::STTCapabilities capabilities() const override {
        return {
            .supports_streaming = true,
            .supports_interim_results = true,
            .supports_word_timestamps = true,
            .supports_multilingual = true,
            .is_offline_capable = true,
            .typical_latency_ms = 10,
            .supported_languages = {"en", "hi", "hinglish"}
        };
    }
    [[nodiscard]] bool is_healthy() const override { return is_healthy_; }

private:
    std::string canned_text_;
    contracts::STTConfig config_;
    contracts::TranscriptCallback callback_;
    bool is_streaming_{false};
    bool is_healthy_{true};
};

} // namespace vani::adapters::stt
