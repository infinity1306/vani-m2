#pragma once

#include "gated_voice_pipeline.hpp"
#include "../session/voice_session_manager.hpp"
#include "../tts/tts_manager.hpp"
#include "../../audio/output/miniaudio_audio_output.hpp"
#include "../../capabilities/system/gateway/tool_gateway.hpp"
#include "../../contracts/common/cancellation_token.hpp"
#include "../telemetry/voice_telemetry.hpp"

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>

namespace vani::voice::pipeline {

enum class VoiceLoopState {
    Idle,
    WakeDetected,
    Listening,
    Processing,
    Executing,
    Speaking,
    Cancelled,
    Error
};

inline std::string voice_loop_state_to_string(VoiceLoopState s) {
    switch (s) {
        case VoiceLoopState::Idle: return "IDLE";
        case VoiceLoopState::WakeDetected: return "WAKE_DETECTED";
        case VoiceLoopState::Listening: return "LISTENING";
        case VoiceLoopState::Processing: return "PROCESSING";
        case VoiceLoopState::Executing: return "EXECUTING";
        case VoiceLoopState::Speaking: return "SPEAKING";
        case VoiceLoopState::Cancelled: return "CANCELLED";
        case VoiceLoopState::Error: return "ERROR";
        default: return "UNKNOWN";
    }
}

struct VoiceLoopTurnEvidence {
    std::string turn_id;
    bool wake_success{false};
    std::string wake_phrase;
    bool stt_success{false};
    std::string raw_transcript;
    std::string normalized_transcript;
    bool intent_success{false};
    std::string detected_intent;
    bool execution_success{false};
    std::string tool_output;
    bool verification_success{false};
    std::string response_text;
    bool tts_success{false};
    bool playback_success{false};
    bool full_e2e_success{false};

    telemetry::VoiceTurnTimestamps timestamps;
    std::string failure_reason;
};

using TurnCompletedCallback = std::function<void(const VoiceLoopTurnEvidence& evidence)>;

class EndToEndVoiceLoop {
public:
    EndToEndVoiceLoop(
        GatedVoicePipelinePtr pipeline,
        capabilities::system::ToolGatewayPtr gateway,
        tts::TTSManagerPtr tts_manager,
        audio::AudioOutputPtr audio_output
    );
    ~EndToEndVoiceLoop();

    // Ingest continuous microphone frame (e.g., 480 samples @ 16kHz)
    void process_audio_frame(std::span<const float> frame);

    // Process a full turn from audio buffer (for batch/deterministic evaluation)
    VoiceLoopTurnEvidence process_utterance(
        std::span<const float> utterance,
        const std::string& simulated_wake_word = "vani",
        const std::string& simulated_transcript = ""
    );

    // Explicitly trigger wake detection
    void trigger_wake(const std::string& wake_word = "vani");

    // Finalize current listening session
    void finalize_turn();

    // Cancel current active operation & flush audio immediately (barge-in)
    void cancel();

    // Reset pipeline state back to IDLE
    void reset();

    void set_turn_callback(TurnCompletedCallback cb);
    void set_agent_controller(std::shared_ptr<void> controller); // Forward-compatible pointer
    void handle_intent_ready(const GatedTurnResult& turn);

    [[nodiscard]] VoiceLoopState state() const;
    [[nodiscard]] size_t total_turns_processed() const;
    [[nodiscard]] size_t successful_turns_count() const;
    [[nodiscard]] size_t self_trigger_suppression_count() const;
    [[nodiscard]] size_t whisper_invocations_count() const;

private:
    std::string generate_response_text(const std::string& intent_name, bool is_success, const std::string& original_text);

    mutable std::mutex mutex_;
    GatedVoicePipelinePtr pipeline_;
    capabilities::system::ToolGatewayPtr gateway_;
    tts::TTSManagerPtr tts_manager_;
    audio::AudioOutputPtr audio_output_;
    std::shared_ptr<void> agent_controller_{nullptr};

    std::atomic<VoiceLoopState> state_{VoiceLoopState::Idle};
    std::atomic<bool> is_speaking_{false};
    std::atomic<size_t> self_trigger_suppressions_{0};
    std::atomic<size_t> total_turns_{0};
    std::atomic<size_t> successful_turns_{0};

    TurnCompletedCallback turn_callback_;
    contracts::CancellationSource active_cancellation_source_;
    VoiceLoopTurnEvidence current_turn_evidence_;
};

using EndToEndVoiceLoopPtr = std::shared_ptr<EndToEndVoiceLoop>;

} // namespace vani::voice::pipeline
