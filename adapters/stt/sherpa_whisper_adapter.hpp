#pragma once

#include "../../contracts/providers/stt_engine.hpp"
#include <memory>
#include <string>

namespace vani::adapters::stt {

/**
 * @brief Real Sherpa-ONNX Whisper STT Adapter.
 *
 * Implements high-accuracy multilingual speech recognition using OpenAI Whisper Tiny/Base
 * ONNX models via native Sherpa-ONNX C-API.
 */
class SherpaWhisperAdapter : public contracts::STTEngine {
public:
    struct Options {
        std::string encoder_path{"models/sherpa-onnx-whisper-tiny/tiny-encoder.int8.onnx"};
        std::string decoder_path{"models/sherpa-onnx-whisper-tiny/tiny-decoder.int8.onnx"};
        std::string tokens_path{"models/sherpa-onnx-whisper-tiny/tiny-tokens.txt"};
        std::string language{"auto"}; // auto, hi, en, etc.
        std::string task{"transcribe"};
        int32_t sample_rate{16000};
        int32_t feature_dim{80};
        int32_t num_threads{2};
        std::string provider{"cpu"};
    };

    SherpaWhisperAdapter();
    explicit SherpaWhisperAdapter(const Options& options);
    ~SherpaWhisperAdapter() override;

    // Non-copyable, movable
    SherpaWhisperAdapter(const SherpaWhisperAdapter&) = delete;
    SherpaWhisperAdapter& operator=(const SherpaWhisperAdapter&) = delete;
    SherpaWhisperAdapter(SherpaWhisperAdapter&&) noexcept;
    SherpaWhisperAdapter& operator=(SherpaWhisperAdapter&&) noexcept;

    // contracts::STTEngine interface
    contracts::Result<void> start_stream(
        const contracts::STTConfig& config,
        contracts::TranscriptCallback callback
    ) override;

    contracts::Result<void> push_audio(
        std::span<const float> samples
    ) override;

    contracts::Result<void> stop_stream() override;

    [[nodiscard]] std::string engine_name() const override;
    [[nodiscard]] contracts::STTCapabilities capabilities() const override;
    [[nodiscard]] bool is_healthy() const override;
    [[nodiscard]] bool is_model_loaded() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace vani::adapters::stt
