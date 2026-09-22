#pragma once

#include "../../contracts/providers/tts_engine.hpp"
#include <string>
#include <vector>
#include <memory>
#include <mutex>

struct SherpaOnnxOfflineTts;

namespace vani::adapters::tts {

struct MatchaTTSModelConfig {
    std::string acoustic_model_path;
    std::string vocoder_path;
    std::string tokens_path;
    std::string data_dir;
    int32_t num_threads{2};
    float noise_scale{0.667f};
    float length_scale{1.0f};
    std::string voice_name{"en_US-ljspeech"};
    std::string language{"en"};
    uint32_t sample_rate{22050};
};

class RealMatchaTTSAdapter : public contracts::TTSEngine {
public:
    explicit RealMatchaTTSAdapter(const MatchaTTSModelConfig& config);
    ~RealMatchaTTSAdapter() override;

    RealMatchaTTSAdapter(const RealMatchaTTSAdapter&) = delete;
    RealMatchaTTSAdapter& operator=(const RealMatchaTTSAdapter&) = delete;
    RealMatchaTTSAdapter(RealMatchaTTSAdapter&&) noexcept;
    RealMatchaTTSAdapter& operator=(RealMatchaTTSAdapter&&) noexcept;

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

    [[nodiscard]] const MatchaTTSModelConfig& model_config() const { return config_; }

private:
    MatchaTTSModelConfig config_;
    const SherpaOnnxOfflineTts* tts_handle_{nullptr};
    bool initialized_{false};
    mutable std::mutex mutex_;
};

} // namespace vani::adapters::tts
