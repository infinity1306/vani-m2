#pragma once

#include "voice_session.hpp"
#include "voice_state.hpp"
#include "../../contracts/common/result.hpp"
#include <mutex>
#include <functional>
#include <memory>
#include <optional>

namespace vani::voice::session {

using StateChangeCallback = std::function<void(VoiceState old_state, VoiceState new_state)>;
using BargeInCallback = std::function<void(const std::string& session_id)>;

class VoiceSessionManager {
public:
    VoiceSessionManager();
    ~VoiceSessionManager() = default;

    contracts::Result<std::string> start_session(
        const std::string& input_device_id = "default",
        const std::string& output_device_id = "default"
    );

    contracts::Result<void> end_session();

    contracts::Result<void> transition_to(VoiceState new_state);

    contracts::Result<void> trigger_barge_in();

    [[nodiscard]] VoiceState current_state() const;
    [[nodiscard]] std::optional<VoiceSession> current_session() const;
    [[nodiscard]] bool is_active() const;

    void set_state_callback(StateChangeCallback callback);
    void set_barge_in_callback(BargeInCallback callback);

private:
    bool is_valid_transition(VoiceState from, VoiceState to) const noexcept;

    mutable std::mutex mutex_;
    std::optional<VoiceSession> active_session_;
    VoiceState state_{VoiceState::Idle};
    StateChangeCallback state_callback_;
    BargeInCallback barge_in_callback_;
    uint64_t session_counter_{0};
};

using VoiceSessionManagerPtr = std::shared_ptr<VoiceSessionManager>;

} // namespace vani::voice::session
