#pragma once

#include "voice_state.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace vani::voice::session {

struct VoiceSession {
    std::string session_id;
    uint64_t started_at_ms{0};
    uint64_t ended_at_ms{0};
    std::string input_device_id;
    std::string output_device_id;
    std::string detected_wake_word;
    std::string active_language{"en"};
    VoiceState state{VoiceState::Idle};
    std::vector<std::string> transcript_ids;
    uint32_t turn_count{0};
    bool is_barge_in_active{false};
    std::string error_message;
};

} // namespace vani::voice::session
