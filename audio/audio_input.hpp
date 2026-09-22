#pragma once

#include "audio_format.hpp"
#include "audio_device.hpp"
#include "../contracts/common/result.hpp"
#include <span>
#include <functional>
#include <memory>

namespace vani::audio {

using AudioCallback = std::function<void(std::span<const float> samples, const AudioFrame& frame_meta)>;

class AudioInput {
public:
    virtual ~AudioInput() = default;

    virtual contracts::Result<void> start() = 0;
    virtual contracts::Result<void> stop() = 0;
    virtual contracts::Result<void> set_callback(AudioCallback callback) = 0;

    [[nodiscard]] virtual AudioFormat format() const = 0;
    [[nodiscard]] virtual bool is_capturing() const = 0;
    [[nodiscard]] virtual std::string device_name() const = 0;
};

using AudioInputPtr = std::shared_ptr<AudioInput>;

} // namespace vani::audio
