#pragma once

#include "../../contracts/providers/stt_engine.hpp"
#include <memory>
#include <string>

namespace vani::adapters::stt {

/**
 * @brief Real Sherpa-ONNX Streaming STT Adapter.
 *
 * Implements low-latency streaming speech recognition using Zipformer Transducer
 * models via native C-API inference.
 * The neural model is loaded and warmed up once at initialization (warm provider).
 * Streaming states are managed per audio stream without reloading model weights.
 */
class RealSherpaSTTAdapter : public contracts::STTEngine {
public:
    struct Options {
        std::string tokens_path{"models/sherpa-onnx-streaming-zipformer-en-20M-2023-02-17/tokens.txt"};
        std::string encoder_path{"models/sherpa-onnx-streaming-zipformer-en-20M-2023-02-17/encoder-epoch-99-avg-1.int8.onnx"};
        std::string decoder_path{"models/sherpa-onnx-streaming-zipformer-en-20M-2023-02-17/decoder-epoch-99-avg-1.int8.onnx"};
        std::string joiner_path{"models/sherpa-onnx-streaming-zipformer-en-20M-2023-02-17/joiner-epoch-99-avg-1.onnx"};
        int32_t sample_rate{16000};
        int32_t feature_dim{80};
        int32_t num_threads{2};
        std::string provider{"cpu"};
        std::string decoding_method{"greedy_search"};
        int32_t max_active_paths{4};
        bool enable_endpoint{true};
    };

    RealSherpaSTTAdapter();
    explicit RealSherpaSTTAdapter(const Options& options);
    ~RealSherpaSTTAdapter() override;

    // Non-copyable, movable
    RealSherpaSTTAdapter(const RealSherpaSTTAdapter&) = delete;
    RealSherpaSTTAdapter& operator=(const RealSherpaSTTAdapter&) = delete;
    RealSherpaSTTAdapter(RealSherpaSTTAdapter&&) noexcept;
    RealSherpaSTTAdapter& operator=(RealSherpaSTTAdapter&&) noexcept;

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
