#pragma once

#include "vad_result.hpp"
#include <span>
#include <memory>
#include <string>

namespace vani::audio::vad {

struct VADConfig {
    float positive_speech_threshold{0.5f};
    float negative_speech_threshold{0.35f};
    uint32_t min_speech_duration_ms{100};
    uint32_t min_silence_duration_ms{300};
    uint32_t sample_rate{16000};
};

class VADEngine {
public:
    virtual ~VADEngine() = default;

    virtual VADResult process(std::span<const float> audio_frame) = 0;
    virtual void reset() = 0;

    [[nodiscard]] virtual std::string engine_name() const = 0;
    [[nodiscard]] virtual const VADConfig& config() const = 0;
    virtual void set_config(const VADConfig& config) = 0;
};

using VADEnginePtr = std::shared_ptr<VADEngine>;

} // namespace vani::audio::vad
