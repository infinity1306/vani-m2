#pragma once

#include <cstdint>
#include <string>

namespace vani::audio::vad {

enum class VADState : uint8_t {
    Silence,
    SpeechStarted,
    SpeechContinuing,
    SpeechEnded
};

struct VADResult {
    VADState state{VADState::Silence};
    float speech_probability{0.0f};
    uint64_t timestamp_ms{0};
    uint32_t speech_duration_ms{0};
    uint32_t silence_duration_ms{0};
    bool is_speech{false};
};

} // namespace vani::audio::vad
