#pragma once

#include <string>
#include <cstdint>

namespace vani::audio::wakeword {

enum class WakeWordState : uint8_t {
    Idle,
    WakeDetected,
    Listening,
    Processing,
    Responding,
    Timeout,
    Disabled
};

struct DetectionResult {
    bool detected{false};
    std::string wake_word;
    float confidence{0.0f};
    uint64_t timestamp_ms{0};
    WakeWordState current_state{WakeWordState::Idle};
};

} // namespace vani::audio::wakeword
