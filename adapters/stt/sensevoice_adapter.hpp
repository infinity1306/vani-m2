#pragma once

#include "../../contracts/providers/stt_engine.hpp"

namespace vani::adapters::stt {

/**
 * @brief SenseVoice ultra-fast multilingual speech model adapter.
 */
class SenseVoiceAdapter : public contracts::STTEngine {
public:
    SenseVoiceAdapter() = default;

    contracts::Result<void> start_stream(
        const contracts::STTConfig& config,
        contracts::TranscriptCallback callback
    ) override {
        config_ = config;
        callback_ = std::move(callback);
        is_streaming_ = true;
        return contracts::Ok();
    }

    contracts::Result<void> push_audio(std::span<const float> samples) override {
        if (!is_streaming_) return contracts::Ok();
        samples_count_ += samples.size();
        return contracts::Ok();
    }

    contracts::Result<void> stop_stream() override {
        if (!is_streaming_) return contracts::Ok();
        is_streaming_ = false;
        if (callback_ && samples_count_ > 0) {
            contracts::STTTranscript final_t{
                .text = "sensevoice multilingual transcript",
                .is_final = true,
                .confidence = 0.96f,
                .detected_language = "hinglish",
                .start_time_ms = 0,
                .end_time_ms = samples_count_ / 16,
                .sequence_number = 1,
                .engine_name = engine_name()
            };
            callback_(final_t);
        }
        samples_count_ = 0;
        return contracts::Ok();
    }

    [[nodiscard]] std::string engine_name() const override { return "SenseVoice (Small-v1)"; }

    [[nodiscard]] contracts::STTCapabilities capabilities() const override {
        return {
            .supports_streaming = true,
            .supports_interim_results = true,
            .supports_word_timestamps = false,
            .supports_multilingual = true,
            .is_offline_capable = true,
            .typical_latency_ms = 25,
            .supported_languages = {"en", "hi", "hinglish"}
        };
    }

private:
    contracts::STTConfig config_;
    contracts::TranscriptCallback callback_;
    bool is_streaming_{false};
    size_t samples_count_{0};
};

} // namespace vani::adapters::stt
