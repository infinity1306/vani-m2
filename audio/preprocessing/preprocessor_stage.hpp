#pragma once

#include "../audio_format.hpp"
#include <vector>
#include <span>
#include <string>
#include <memory>

namespace vani::audio::preprocessing {

enum class StageType : uint8_t {
    NoiseSuppression,
    EchoCancellation,
    GainNormalization,
    Resampling,
    Custom
};

class PreprocessorStage {
public:
    virtual ~PreprocessorStage() = default;

    virtual std::vector<float> process(std::span<const float> input, const AudioFormat& format) = 0;
    [[nodiscard]] virtual std::string stage_name() const = 0;
    [[nodiscard]] virtual StageType type() const = 0;
    [[nodiscard]] virtual bool is_enabled() const = 0;
    virtual void set_enabled(bool enabled) = 0;
};

using PreprocessorStagePtr = std::shared_ptr<PreprocessorStage>;

} // namespace vani::audio::preprocessing
