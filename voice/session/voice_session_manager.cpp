#include "voice_session_manager.hpp"
#include <chrono>

namespace vani::voice::session {

VoiceSessionManager::VoiceSessionManager() = default;

bool VoiceSessionManager::is_valid_transition(VoiceState from, VoiceState to) const noexcept {
    if (from == to) return true;
    if (to == VoiceState::Error || to == VoiceState::Disabled) return true;

    switch (from) {
        case VoiceState::Disabled:
            return to == VoiceState::Idle;
        case VoiceState::Idle:
            return to == VoiceState::Arming || to == VoiceState::Listening || to == VoiceState::Offline;
        case VoiceState::Arming:
            return to == VoiceState::Listening || to == VoiceState::Idle;
        case VoiceState::Listening:
            return to == VoiceState::Transcribing || to == VoiceState::Idle;
        case VoiceState::Transcribing:
            return to == VoiceState::Understanding || to == VoiceState::Listening || to == VoiceState::Idle;
        case VoiceState::Understanding:
            return to == VoiceState::Executing || to == VoiceState::WaitingForInput || to == VoiceState::WaitingForPermission || to == VoiceState::Speaking || to == VoiceState::Idle;
        case VoiceState::Executing:
            return to == VoiceState::Speaking || to == VoiceState::WaitingForInput || to == VoiceState::WaitingForPermission || to == VoiceState::Idle;
        case VoiceState::Speaking:
            return to == VoiceState::Idle || to == VoiceState::Interrupted || to == VoiceState::Listening;
        case VoiceState::Interrupted:
            return to == VoiceState::Listening || to == VoiceState::Idle;
        case VoiceState::WaitingForInput:
            return to == VoiceState::Listening || to == VoiceState::Idle;
        case VoiceState::WaitingForPermission:
            return to == VoiceState::Executing || to == VoiceState::Idle;
        case VoiceState::Offline:
            return to == VoiceState::Idle;
        case VoiceState::Error:
            return to == VoiceState::Idle;
    }
    return false;
}

contracts::Result<std::string> VoiceSessionManager::start_session(
    const std::string& input_device_id,
    const std::string& output_device_id
) {
    std::lock_guard<std::mutex> lock(mutex_);
    session_counter_++;
    std::string sid = "vsess-" + std::to_string(session_counter_);

    auto now = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    active_session_ = VoiceSession{
        .session_id = sid,
        .started_at_ms = now,
        .input_device_id = input_device_id,
        .output_device_id = output_device_id,
        .state = VoiceState::Listening,
        .turn_count = 1
    };

    state_ = VoiceState::Listening;
    if (state_callback_) {
        state_callback_(VoiceState::Idle, VoiceState::Listening);
    }

    return contracts::Ok(sid);
}

contracts::Result<void> VoiceSessionManager::end_session() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_session_) return contracts::Ok();

    auto old = state_;
    state_ = VoiceState::Idle;
    active_session_->state = VoiceState::Idle;
    active_session_->ended_at_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    if (state_callback_) {
        state_callback_(old, VoiceState::Idle);
    }
    return contracts::Ok();
}

contracts::Result<void> VoiceSessionManager::transition_to(VoiceState new_state) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_valid_transition(state_, new_state)) {
        return contracts::Fail(
            contracts::ErrorCode::InvalidState,
            "Illegal voice state transition: " + std::string(to_string(state_)) + " -> " + std::string(to_string(new_state))
        );
    }

    auto old = state_;
    state_ = new_state;
    if (active_session_) {
        active_session_->state = new_state;
    }

    if (state_callback_) {
        state_callback_(old, new_state);
    }
    return contracts::Ok();
}

contracts::Result<void> VoiceSessionManager::trigger_barge_in() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == VoiceState::Speaking) {
        auto old = state_;
        state_ = VoiceState::Interrupted;
        if (active_session_) {
            active_session_->is_barge_in_active = true;
            active_session_->state = VoiceState::Interrupted;
        }

        if (barge_in_callback_ && active_session_) {
            barge_in_callback_(active_session_->session_id);
        }

        if (state_callback_) {
            state_callback_(old, VoiceState::Interrupted);
        }

        // Immediately transition to Listening
        state_ = VoiceState::Listening;
        if (state_callback_) {
            state_callback_(VoiceState::Interrupted, VoiceState::Listening);
        }
    }
    return contracts::Ok();
}

VoiceState VoiceSessionManager::current_state() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

std::optional<VoiceSession> VoiceSessionManager::current_session() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_session_;
}

bool VoiceSessionManager::is_active() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_session_.has_value() && state_ != VoiceState::Idle && state_ != VoiceState::Disabled;
}

void VoiceSessionManager::set_state_callback(StateChangeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_callback_ = std::move(callback);
}

void VoiceSessionManager::set_barge_in_callback(BargeInCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    barge_in_callback_ = std::move(callback);
}

} // namespace vani::voice::session
