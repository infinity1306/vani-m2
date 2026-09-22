#pragma once

#include "../../audio/wakeword/wakeword_engine.hpp"
#include "../../contracts/providers/stt_engine.hpp"
#include "../../adapters/vad/real_silero_vad_adapter.hpp"
#include "../../audio/vad/adaptive_vad.hpp"
#include "../normalization/language_normalizer.hpp"
#include "../intent/intent_preparer.hpp"
#include "../telemetry/voice_telemetry.hpp"
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <functional>
#include <chrono>

namespace vani::voice::pipeline {

enum class PipelineState {
    Idle,
    WakeDetected,
    Listening,
    ProcessingSTT,
    IntentReady,
    Timeout,
    Cancelled,
    Error
};

inline std::string pipeline_state_to_string(PipelineState s) {
    switch (s) {
        case PipelineState::Idle: return "IDLE";
        case PipelineState::WakeDetected: return "WAKE_DETECTED";
        case PipelineState::Listening: return "LISTENING";
        case PipelineState::ProcessingSTT: return "PROCESSING_STT";
        case PipelineState::IntentReady: return "INTENT_READY";
        case PipelineState::Timeout: return "TIMEOUT";
        case PipelineState::Cancelled: return "CANCELLED";
        case PipelineState::Error: return "ERROR";
        default: return "UNKNOWN";
    }
}

struct GatedPipelineConfig {
    uint32_t sample_rate{16000};
    uint32_t pre_roll_ms{600};          // 600ms pre-roll audio history
    uint32_t listen_timeout_ms{4500};    // 4.5s maximum command listening window
    uint32_t max_trailing_silence_ms{1200}; // VAD speech endpoint threshold
    bool enable_audio_gating{true};
    audio::vad::AdaptiveVadConfig vad_config{};
};

struct GatedTurnResult {
    std::string turn_id;
    bool wake_triggered{false};
    std::string wake_phrase;
    std::string raw_transcript;
    std::string normalized_text;
    std::string detected_intent;
    telemetry::VoiceTurnTimestamps timestamps;
    bool is_timeout{false};
    bool is_cancelled{false};
};

using IntentCallback = std::function<void(const GatedTurnResult& result)>;

class GatedVoicePipeline {
public:
    GatedVoicePipeline(
        audio::wakeword::WakeWordEnginePtr wakeword_engine,
        contracts::STTEnginePtr stt_engine,
        const GatedPipelineConfig& config = {}
    );
    ~GatedVoicePipeline();

    // Feeds incoming continuous microphone frame (e.g. 480 samples = 30ms @ 16kHz)
    void process_audio_frame(std::span<const float> frame);

    void cancel();
    void reset();

    void open_voice_session(const std::string& wake_word, uint64_t wake_time_ns);
    void finalize_voice_session();
    void set_simulated_transcript(const std::string& transcript);

    void set_intent_callback(IntentCallback cb);
    [[nodiscard]] PipelineState current_state() const;
    [[nodiscard]] size_t whisper_invocations_count() const;
    [[nodiscard]] size_t wake_detections_count() const;
    [[nodiscard]] size_t timeout_count() const;

    // Phase 7C.1 Adaptive VAD Telemetry
    [[nodiscard]] float noise_floor_rms() const;
    [[nodiscard]] float adaptive_vad_threshold() const;
    [[nodiscard]] float current_frame_rms() const;
    [[nodiscard]] uint64_t speech_frame_count() const;
    [[nodiscard]] uint64_t silence_frame_count() const;
    [[nodiscard]] uint64_t speech_start_timestamp_ns() const;
    [[nodiscard]] uint64_t speech_end_timestamp_ns() const;
    [[nodiscard]] bool speech_detected_in_session() const;

private:
    mutable std::recursive_mutex mutex_;
    audio::wakeword::WakeWordEnginePtr wakeword_;
    contracts::STTEnginePtr stt_;
    normalization::LanguageNormalizer normalizer_;
    intent::IntentPreparer intent_preparer_;
    GatedPipelineConfig config_;
    audio::vad::AdaptiveVadDetector vad_detector_;

    PipelineState state_{PipelineState::Idle};
    IntentCallback intent_callback_;

    // Bounded pre-roll circular buffer
    std::vector<float> pre_roll_buffer_;
    size_t pre_roll_capacity_samples_{9600}; // 600ms @ 16kHz
    size_t pre_roll_write_pos_{0};
    bool pre_roll_full_{false};

    // Active session state
    std::string current_turn_id_;
    std::string current_wake_word_;
    telemetry::VoiceTurnTimestamps current_timestamps_;
    std::chrono::steady_clock::time_point session_start_time_;
    std::chrono::steady_clock::time_point last_speech_time_;
    bool speech_detected_in_session_{false};
    std::string last_partial_text_;
    std::string final_stt_text_;
    std::string simulated_transcript_;

    // Observability metrics
    size_t whisper_invocations_{0};
    size_t wake_detections_{0};
    size_t timeouts_{0};
};

using GatedVoicePipelinePtr = std::shared_ptr<GatedVoicePipeline>;

} // namespace vani::voice::pipeline
