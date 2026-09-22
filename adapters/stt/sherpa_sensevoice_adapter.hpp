#pragma once

#include "../../contracts/providers/stt_engine.hpp"
#include <memory>
#include <string>

namespace vani::adapters::stt {

/**
 * @brief Real Sherpa-ONNX SenseVoice-Small STT Adapter.
 *
 * Implements high-accuracy multilingual speech recognition with audio event / emotion detection
 * using SenseVoice-Small ONNX model via native Sherpa-ONNX C-API.
 */
class SherpaSenseVoiceAdapter : public contracts::STTEngine {
public:
    struct Options {
        std::string model_path{"models/sherpa-onnx-sense-voice/model.int8.onnx"};
        std::string tokens_path{"models/sherpa-onnx-sense-voice/tokens.txt"};
        std::string language{"auto"}; // auto, zh, en, yue, ja, ko
        bool use_itn{true};
        int32_t sample_rate{16000};
        int32_t feature_dim{80};
        int32_t num_threads{2};
        std::string provider{"cpu"};
    };

    SherpaSenseVoiceAdapter();
    explicit SherpaSenseVoiceAdapter(const Options& options);
    ~SherpaSenseVoiceAdapter() override;

    // Non-copyable, movable
    SherpaSenseVoiceAdapter(const SherpaSenseVoiceAdapter&) = delete;
    SherpaSenseVoiceAdapter& operator=(const SherpaSenseVoiceAdapter&) = delete;
    SherpaSenseVoiceAdapter(SherpaSenseVoiceAdapter&&) noexcept;
    SherpaSenseVoiceAdapter& operator=(SherpaSenseVoiceAdapter&&) noexcept;

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
