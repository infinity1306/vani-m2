#pragma once

#include "../../contracts/providers/tts_engine.hpp"
#include <string>
#include <vector>
#include <memory>
#include <mutex>

// Forward declaration of sherpa-onnx types
struct SherpaOnnxOfflineTts;

namespace vani::adapters::tts {

struct PiperTTSModelConfig {
    std::string model_path;
    std::string tokens_path;
    std::string data_dir;
    std::string lexicon_path;
    int32_t num_threads{2};
    float noise_scale{0.667f};
    float noise_scale_w{0.8f};
    float length_scale{1.0f};
    std::string voice_name{"en_US-lessac"};
    std::string language{"en"};
    uint32_t sample_rate{22050};
};

class RealPiperTTSAdapter : public contracts::TTSEngine {
public:
    explicit RealPiperTTSAdapter(const PiperTTSModelConfig& config);
    ~RealPiperTTSAdapter() override;

    // Disallow copy, allow move
    RealPiperTTSAdapter(const RealPiperTTSAdapter&) = delete;
    RealPiperTTSAdapter& operator=(const RealPiperTTSAdapter&) = delete;
    RealPiperTTSAdapter(RealPiperTTSAdapter&&) noexcept;
    RealPiperTTSAdapter& operator=(RealPiperTTSAdapter&&) noexcept;

    contracts::Result<std::vector<float>> synthesize(
        const std::string& text,
        const contracts::TTSConfig& config = {},
        contracts::CancellationToken cancellation_token = contracts::CancellationToken::none()
    ) override;

    contracts::Result<void> synthesize_stream(
        const std::string& text,
        const contracts::TTSConfig& config,
        contracts::AudioChunkCallback chunk_cb,
        contracts::CancellationToken cancellation_token = contracts::CancellationToken::none()
    ) override;

    [[nodiscard]] std::string engine_name() const override;
    [[nodiscard]] contracts::TTSCapabilities capabilities() const override;
    [[nodiscard]] bool is_healthy() const override;

    [[nodiscard]] const PiperTTSModelConfig& model_config() const { return config_; }

private:
    PiperTTSModelConfig config_;
    const SherpaOnnxOfflineTts* tts_handle_{nullptr};
    bool initialized_{false};
    mutable std::mutex mutex_;
};

} // namespace vani::adapters::tts
