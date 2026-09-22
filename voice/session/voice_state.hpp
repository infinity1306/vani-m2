#pragma once

#include <cstdint>
#include <string_view>

namespace vani::voice::session {

enum class VoiceState : uint8_t {
    Disabled,
    Idle,
    Arming,
    Listening,
    Transcribing,
    Understanding,
    Executing,
    Speaking,
    Interrupted,
    WaitingForInput,
    WaitingForPermission,
    Offline,
    Error
};

[[nodiscard]] constexpr std::string_view to_string(VoiceState state) noexcept {
    switch (state) {
        case VoiceState::Disabled:              return "DISABLED";
        case VoiceState::Idle:                  return "IDLE";
        case VoiceState::Arming:                return "ARMING";
        case VoiceState::Listening:             return "LISTENING";
        case VoiceState::Transcribing:          return "TRANSCRIBING";
        case VoiceState::Understanding:         return "UNDERSTANDING";
        case VoiceState::Executing:             return "EXECUTING";
        case VoiceState::Speaking:              return "SPEAKING";
        case VoiceState::Interrupted:           return "INTERRUPTED";
        case VoiceState::WaitingForInput:       return "WAITING_FOR_INPUT";
        case VoiceState::WaitingForPermission:  return "WAITING_FOR_PERMISSION";
        case VoiceState::Offline:               return "OFFLINE";
        case VoiceState::Error:                 return "ERROR";
    }
    return "UNKNOWN";
}

} // namespace vani::voice::session
