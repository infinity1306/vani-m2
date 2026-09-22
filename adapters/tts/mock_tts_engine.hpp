#pragma once

#include "../../contracts/providers/tts_engine.hpp"

namespace vani::adapters::tts {

/**
 * @brief Mock TTS Engine for tests.
 */
class MockTTSEngine : public contracts::TTSEngine {
public:
    MockTTSEngine() = default;

    contracts::Result<std::vector<float>> synthesize(
        const std::string& text,
        const contracts::TTSConfig& /*config*/ = {},
        contracts::CancellationToken cancellation_token = contracts::CancellationToken::none()
    ) override {
        if (cancellation_token.is_cancelled()) {
            return contracts::Fail(contracts::ErrorCode::Cancelled, "Mock TTS cancelled");
        }
        last_spoken_text_ = text;
        return contracts::Ok(std::vector<float>(1600, 0.1f));
    }

    contracts::Result<void> synthesize_stream(
        const std::string& text,
        const contracts::TTSConfig& /*config*/,
        contracts::AudioChunkCallback chunk_cb,
        contracts::CancellationToken cancellation_token = contracts::CancellationToken::none()
    ) override {
        last_spoken_text_ = text;
        if (cancellation_token.is_cancelled()) {
            return contracts::Fail(contracts::ErrorCode::Cancelled, "Mock Streaming TTS cancelled");
        }
        if (chunk_cb) {
            std::vector<float> chunk(400, 0.1f);
            chunk_cb(chunk, true);
        }
        return contracts::Ok();
    }

    [[nodiscard]] std::string last_spoken_text() const { return last_spoken_text_; }
    [[nodiscard]] std::string engine_name() const override { return "MockTTS"; }

    [[nodiscard]] contracts::TTSCapabilities capabilities() const override {
        return {
            .supports_streaming = true,
            .supports_ssml = true,
            .is_offline_capable = true,
            .typical_ttfa_ms = 5,
            .supported_voices = {"mock_voice"}
        };
    }

private:
    std::string last_spoken_text_;
};

} // namespace vani::adapters::tts
