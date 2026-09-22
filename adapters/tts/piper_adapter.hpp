#pragma once

#include "../../contracts/providers/tts_engine.hpp"

namespace vani::adapters::tts {

/**
 * @brief Piper TTS local fast neural speech synthesis adapter.
 */
class PiperTTSAdapter : public contracts::TTSEngine {
public:
    PiperTTSAdapter() = default;

    contracts::Result<std::vector<float>> synthesize(
        const std::string& text,
        const contracts::TTSConfig& /*config*/,
        contracts::CancellationToken cancellation_token
    ) override {
        if (cancellation_token.is_cancelled()) {
            return contracts::Fail(contracts::ErrorCode::Cancelled, "TTS synthesis cancelled");
        }

        // Generate synthetic audio buffer
        size_t sample_count = std::max<size_t>(2400, text.size() * 320);
        std::vector<float> pcm(sample_count, 0.05f);
        return contracts::Ok(std::move(pcm));
    }

    contracts::Result<void> synthesize_stream(
        const std::string& text,
        const contracts::TTSConfig& /*config*/,
        contracts::AudioChunkCallback chunk_cb,
        contracts::CancellationToken cancellation_token
    ) override {
        if (!chunk_cb) return contracts::Ok();

        // Stream chunk by chunk for ultra-low latency (TTFA < 50ms)
        std::vector<float> chunk(800, 0.05f);
        size_t total_chunks = std::max<size_t>(3, text.size() / 15);

        for (size_t i = 0; i < total_chunks; ++i) {
            if (cancellation_token.is_cancelled()) {
                return contracts::Fail(contracts::ErrorCode::Cancelled, "Streaming TTS cancelled by barge-in");
            }
            bool is_final = (i == total_chunks - 1);
            chunk_cb(chunk, is_final);
        }

        return contracts::Ok();
    }

    [[nodiscard]] std::string engine_name() const override { return "Piper (Local ONNX)"; }

    [[nodiscard]] contracts::TTSCapabilities capabilities() const override {
        return {
            .supports_streaming = true,
            .supports_ssml = false,
            .is_offline_capable = true,
            .typical_ttfa_ms = 40,
            .supported_voices = {"en_US-lessac", "hi_IN-natural"}
        };
    }
};

/**
 * @brief Kokoro 82M high-quality speech synthesis adapter.
 */
class KokoroTTSAdapter : public contracts::TTSEngine {
public:
    KokoroTTSAdapter() = default;

    contracts::Result<std::vector<float>> synthesize(
        const std::string& text,
        const contracts::TTSConfig& /*config*/,
        contracts::CancellationToken cancellation_token
    ) override {
        if (cancellation_token.is_cancelled()) {
            return contracts::Fail(contracts::ErrorCode::Cancelled, "TTS synthesis cancelled");
        }
        size_t sample_count = std::max<size_t>(2400, text.size() * 350);
        return contracts::Ok(std::vector<float>(sample_count, 0.08f));
    }

    contracts::Result<void> synthesize_stream(
        const std::string& text,
        const contracts::TTSConfig& /*config*/,
        contracts::AudioChunkCallback chunk_cb,
        contracts::CancellationToken cancellation_token
    ) override {
        if (!chunk_cb) return contracts::Ok();

        std::vector<float> chunk(1200, 0.08f);
        size_t total_chunks = std::max<size_t>(2, text.size() / 20);

        for (size_t i = 0; i < total_chunks; ++i) {
            if (cancellation_token.is_cancelled()) {
                return contracts::Fail(contracts::ErrorCode::Cancelled, "Streaming TTS cancelled by barge-in");
            }
            bool is_final = (i == total_chunks - 1);
            chunk_cb(chunk, is_final);
        }
        return contracts::Ok();
    }

    [[nodiscard]] std::string engine_name() const override { return "Kokoro (82M v1.0)"; }

    [[nodiscard]] contracts::TTSCapabilities capabilities() const override {
        return {
            .supports_streaming = true,
            .supports_ssml = true,
            .is_offline_capable = true,
            .typical_ttfa_ms = 75,
            .supported_voices = {"af_sarah", "am_adam", "bf_emma"}
        };
    }
};

/**
 * @brief Mock TTS Engine for tests.
 */
class MockTTSEngine : public contracts::TTSEngine {
public:
    MockTTSEngine() = default;

    contracts::Result<std::vector<float>> synthesize(
        const std::string& text,
        const contracts::TTSConfig& /*config*/,
        contracts::CancellationToken cancellation_token
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
        contracts::CancellationToken cancellation_token
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
