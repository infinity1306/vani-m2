#pragma once

#include "preprocessor_stage.hpp"
#include "../audio_format.hpp"
#include <vector>
#include <memory>
#include <mutex>

namespace vani::audio::preprocessing {

class AudioPreprocessor {
public:
    AudioPreprocessor();
    ~AudioPreprocessor() = default;

    void add_stage(PreprocessorStagePtr stage);
    void clear_stages();

    std::vector<float> process_chain(std::span<const float> input, const AudioFormat& format);

    [[nodiscard]] size_t stage_count() const;

private:
    std::vector<PreprocessorStagePtr> stages_;
    mutable std::mutex mutex_;
};

// Built-in standard stages
class GainNormalizationStage : public PreprocessorStage {
public:
    explicit GainNormalizationStage(float target_rms = 0.1f) : target_rms_(target_rms) {}

    std::vector<float> process(std::span<const float> input, const AudioFormat& format) override;
    [[nodiscard]] std::string stage_name() const override { return "GainNormalization"; }
    [[nodiscard]] StageType type() const override { return StageType::GainNormalization; }
    [[nodiscard]] bool is_enabled() const override { return enabled_; }
    void set_enabled(bool enabled) override { enabled_ = enabled; }

private:
    float target_rms_{0.1f};
    bool enabled_{true};
};

class SimpleNoiseSuppressionStage : public PreprocessorStage {
public:
    explicit SimpleNoiseSuppressionStage(float threshold = 0.015f) : threshold_(threshold) {}

    std::vector<float> process(std::span<const float> input, const AudioFormat& format) override;
    [[nodiscard]] std::string stage_name() const override { return "NoiseSuppression"; }
    [[nodiscard]] StageType type() const override { return StageType::NoiseSuppression; }
    [[nodiscard]] bool is_enabled() const override { return enabled_; }
    void set_enabled(bool enabled) override { enabled_ = enabled; }

private:
    float threshold_{0.015f};
    bool enabled_{true};
};

} // namespace vani::audio::preprocessing
