#pragma once

#include "../../contracts/providers/stt_engine.hpp"

namespace vani::adapters {

class MockSTTAdapter : public contracts::STTEngine {
public:
    contracts::Result<void> start_stream(
        const contracts::STTConfig& /*config*/,
        contracts::TranscriptCallback callback
    ) override {
        callback_ = std::move(callback);
        is_streaming_ = true;
        return contracts::Result<void>::ok();
    }

    contracts::Result<void> push_audio(
        std::span<const float> samples
    ) override {
        if (!is_streaming_) {
            return contracts::Result<void>::err(
                contracts::ErrorCode::Failure,
                "Cannot push audio: stream not started",
                "adapter.mock_stt"
            );
        }
        if (callback_) {
            contracts::STTTranscript transcript{
                .text = "mera react project run karde",
                .is_final = samples.size() > 100,
                .confidence = 0.96f,
                .detected_language = "Hinglish"
            };
            callback_(transcript);
        }
        return contracts::Result<void>::ok();
    }

    contracts::Result<void> stop_stream() override {
        is_streaming_ = false;
        callback_ = nullptr;
        return contracts::Result<void>::ok();
    }

    [[nodiscard]] std::string engine_name() const override {
        return "MockSTTAdapter_SherpaV2";
    }

    [[nodiscard]] contracts::STTCapabilities capabilities() const override {
        return {
            .supports_streaming = true,
            .supports_interim_results = true,
            .supports_word_timestamps = false,
            .supports_multilingual = true,
            .is_offline_capable = true,
            .typical_latency_ms = 10,
            .supported_languages = {"en", "hi", "hinglish"}
        };
    }

private:
    bool is_streaming_{false};
    contracts::TranscriptCallback callback_;
};

} // namespace vani::adapters
