#pragma once

#include "detection_result.hpp"
#include "model_metadata.hpp"
#include <span>
#include <memory>
#include <string>
#include <vector>

namespace vani::audio::wakeword {

struct WakeWordConfig {
    std::vector<std::string> target_words{"vani", "hey vani", "ok vani"};
    float detection_threshold{0.65f};
    uint32_t listen_timeout_ms{5000};
    bool enabled{true};
};

class WakeWordEngine {
public:
    virtual ~WakeWordEngine() = default;

    virtual DetectionResult process(std::span<const float> audio_frame) = 0;
    virtual void reset() = 0;

    [[nodiscard]] virtual std::string engine_name() const = 0;
    [[nodiscard]] virtual const WakeWordConfig& config() const = 0;
    virtual void set_config(const WakeWordConfig& config) = 0;
    virtual void set_enabled(bool enabled) = 0;
    [[nodiscard]] virtual bool is_enabled() const = 0;
    [[nodiscard]] virtual bool is_healthy() const { return true; }
    [[nodiscard]] virtual WakeWordModelMetadata metadata() const { return WakeWordModelMetadata{}; }
};

using WakeWordEnginePtr = std::shared_ptr<WakeWordEngine>;

} // namespace vani::audio::wakeword
