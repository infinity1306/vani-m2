#pragma once

#include "../../audio/vad/vad_engine.hpp"
#include <memory>
#include <string>
#include <vector>

namespace vani::adapters::vad {

/**
 * @brief Real Silero VAD Adapter using Sherpa-ONNX C-API.
 *
 * Encapsulates the native Silero VAD neural network inference behind
 * the VANI VADEngine contract. Zero vendor types leak into the core.
 * The ONNX model is loaded once at initialization.
 */
class RealSileroVADAdapter : public audio::vad::VADEngine {
public:
    struct Options {
        std::string model_path{"models/silero_vad.onnx"};
        float threshold{0.5f};
        float min_silence_duration_s{0.3f};
        float min_speech_duration_s{0.1f};
        int32_t window_size{512};
        int32_t sample_rate{16000};
        int32_t num_threads{1};
        std::string provider{"cpu"};
    };

    RealSileroVADAdapter();
    explicit RealSileroVADAdapter(const Options& options);
    ~RealSileroVADAdapter() override;

    // Non-copyable, movable
    RealSileroVADAdapter(const RealSileroVADAdapter&) = delete;
    RealSileroVADAdapter& operator=(const RealSileroVADAdapter&) = delete;
    RealSileroVADAdapter(RealSileroVADAdapter&&) noexcept;
    RealSileroVADAdapter& operator=(RealSileroVADAdapter&&) noexcept;

    // VADEngine interface
    audio::vad::VADResult process(std::span<const float> audio_frame) override;
    void reset() override;

    [[nodiscard]] std::string engine_name() const override;
    [[nodiscard]] const audio::vad::VADConfig& config() const override;
    void set_config(const audio::vad::VADConfig& config) override;

    [[nodiscard]] bool is_model_loaded() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    audio::vad::VADConfig config_;
};

} // namespace vani::adapters::vad
