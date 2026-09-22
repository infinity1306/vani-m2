#pragma once

#include <cstdint>
#include <string>
#include <mutex>
#include <vector>
#include <optional>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace vani::voice::telemetry {

/**
 * @brief High-precision stage timestamps across the entire VANI Voice Turn (T0 through T13).
 * Stages that have not yet occurred are left empty (std::nullopt).
 */
struct VoiceTurnTimestamps {
    std::string turn_id;
    std::string session_id;

    // T0: Audio Capture (Live Mic or Source Buffer)
    std::optional<uint64_t> t0_audio_capture_ns;

    // T1: Speech Detected by VAD
    std::optional<uint64_t> t1_vad_speech_start_ns;

    // T2: Wake-Word Detection
    std::optional<uint64_t> t2_wake_word_detected_ns;

    // T3: First STT Partial Transcript
    std::optional<uint64_t> t3_first_stt_partial_ns;

    // T4: Final STT Result Available
    std::optional<uint64_t> t4_final_stt_result_ns;

    // T5: Language Normalization & Vocabulary Complete
    std::optional<uint64_t> t5_normalization_complete_ns;

    // T6: Intent Formulated
    std::optional<uint64_t> t6_intent_detected_ns;

    // T7: Capability Routing Decision Made
    std::optional<uint64_t> t7_routing_decision_ns;

    // T8: Tool / Capability Execution Start
    std::optional<uint64_t> t8_tool_start_ns;

    // T9: Tool Execution Complete
    std::optional<uint64_t> t9_tool_complete_ns;

    // T10: Postcondition Verification Complete
    std::optional<uint64_t> t10_verification_complete_ns;

    // T11: TTS Synthesis Start
    std::optional<uint64_t> t11_tts_start_ns;

    // T12: First TTS Audio Chunk Produced (TTFA)
    std::optional<uint64_t> t12_first_tts_audio_ns;

    // T13: TTS Synthesis Complete
    std::optional<uint64_t> t13_tts_complete_ns;

    // Phase 6D Extended Telemetry
    // T14: Wake-Word Detected
    std::optional<uint64_t> t14_wake_detected_ns;

    // T15: TTS Request
    std::optional<uint64_t> t15_tts_request_ns;

    // T16: First Audio Output Chunk
    std::optional<uint64_t> t16_first_audio_ns;

    // T17: Synthesis Complete
    std::optional<uint64_t> t17_synthesis_complete_ns;

    // T18: Audio Playback Complete
    std::optional<uint64_t> t18_playback_complete_ns;

    // Flags & Context
    bool was_interrupted{false};
    bool is_fallback{false};
    std::string recognized_text;
    std::string resolved_intent;

    static uint64_t now_ns() {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count()
        );
    }

    void mark_t0() { t0_audio_capture_ns = now_ns(); }
    void mark_t1() { t1_vad_speech_start_ns = now_ns(); }
    void mark_t2() { 
        t2_wake_word_detected_ns = now_ns(); 
        t14_wake_detected_ns = t2_wake_word_detected_ns;
    }
    void mark_t3() { if (!t3_first_stt_partial_ns) t3_first_stt_partial_ns = now_ns(); }
    void mark_t4() { t4_final_stt_result_ns = now_ns(); }
    void mark_t5() { t5_normalization_complete_ns = now_ns(); }
    void mark_t6() { t6_intent_detected_ns = now_ns(); }
    void mark_t7() { t7_routing_decision_ns = now_ns(); }
    void mark_t8() { t8_tool_start_ns = now_ns(); }
    void mark_t9() { t9_tool_complete_ns = now_ns(); }
    void mark_t10() { t10_verification_complete_ns = now_ns(); }
    void mark_t11() { 
        t11_tts_start_ns = now_ns(); 
        t15_tts_request_ns = t11_tts_start_ns;
    }
    void mark_t12() { 
        if (!t12_first_tts_audio_ns) t12_first_tts_audio_ns = now_ns(); 
        if (!t16_first_audio_ns) t16_first_audio_ns = t12_first_tts_audio_ns;
    }
    void mark_t13() { 
        t13_tts_complete_ns = now_ns(); 
        t17_synthesis_complete_ns = t13_tts_complete_ns;
    }
    void mark_t14() { t14_wake_detected_ns = now_ns(); }
    void mark_t15() { t15_tts_request_ns = now_ns(); }
    void mark_t16() { if (!t16_first_audio_ns) t16_first_audio_ns = now_ns(); }
    void mark_t17() { t17_synthesis_complete_ns = now_ns(); }
    void mark_t18() { t18_playback_complete_ns = now_ns(); }

    // Latency Calculations (in milliseconds)
    [[nodiscard]] std::optional<double> audio_to_first_partial_ms() const {
        if (t0_audio_capture_ns && t3_first_stt_partial_ns && *t3_first_stt_partial_ns >= *t0_audio_capture_ns) {
            return static_cast<double>(*t3_first_stt_partial_ns - *t0_audio_capture_ns) / 1e6;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<double> audio_to_final_transcript_ms() const {
        if (t0_audio_capture_ns && t4_final_stt_result_ns && *t4_final_stt_result_ns >= *t0_audio_capture_ns) {
            return static_cast<double>(*t4_final_stt_result_ns - *t0_audio_capture_ns) / 1e6;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<double> wake_to_stt_ms() const {
        uint64_t wake = t14_wake_detected_ns.value_or(t2_wake_word_detected_ns.value_or(0));
        if (wake > 0 && t4_final_stt_result_ns && *t4_final_stt_result_ns >= wake) {
            return static_cast<double>(*t4_final_stt_result_ns - wake) / 1e6;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<double> wake_to_intent_ms() const {
        uint64_t wake = t14_wake_detected_ns.value_or(t2_wake_word_detected_ns.value_or(0));
        if (wake > 0 && t6_intent_detected_ns && *t6_intent_detected_ns >= wake) {
            return static_cast<double>(*t6_intent_detected_ns - wake) / 1e6;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<double> wake_to_tool_complete_ms() const {
        uint64_t wake = t14_wake_detected_ns.value_or(t2_wake_word_detected_ns.value_or(0));
        if (wake > 0 && t9_tool_complete_ns && *t9_tool_complete_ns >= wake) {
            return static_cast<double>(*t9_tool_complete_ns - wake) / 1e6;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<double> wake_to_first_audio_ms() const {
        uint64_t wake = t14_wake_detected_ns.value_or(t2_wake_word_detected_ns.value_or(0));
        uint64_t first_audio = t16_first_audio_ns.value_or(t12_first_tts_audio_ns.value_or(0));
        if (wake > 0 && first_audio >= wake) {
            return static_cast<double>(first_audio - wake) / 1e6;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<double> wake_to_playback_complete_ms() const {
        uint64_t wake = t14_wake_detected_ns.value_or(t2_wake_word_detected_ns.value_or(0));
        uint64_t end_audio = t18_playback_complete_ns.value_or(t17_synthesis_complete_ns.value_or(t13_tts_complete_ns.value_or(0)));
        if (wake > 0 && end_audio >= wake) {
            return static_cast<double>(end_audio - wake) / 1e6;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<double> tts_ttfa_ms() const {
        uint64_t req = t15_tts_request_ns.value_or(t11_tts_start_ns.value_or(0));
        uint64_t first_audio = t16_first_audio_ns.value_or(t12_first_tts_audio_ns.value_or(0));
        if (req > 0 && first_audio >= req) {
            return static_cast<double>(first_audio - req) / 1e6;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<double> transcript_to_intent_ms() const {
        if (t4_final_stt_result_ns && t6_intent_detected_ns && *t6_intent_detected_ns >= *t4_final_stt_result_ns) {
            return static_cast<double>(*t6_intent_detected_ns - *t4_final_stt_result_ns) / 1e6;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<double> intent_to_tool_start_ms() const {
        if (t6_intent_detected_ns && t8_tool_start_ns && *t8_tool_start_ns >= *t6_intent_detected_ns) {
            return static_cast<double>(*t8_tool_start_ns - *t6_intent_detected_ns) / 1e6;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<double> tool_duration_ms() const {
        if (t8_tool_start_ns && t9_tool_complete_ns && *t9_tool_complete_ns >= *t8_tool_start_ns) {
            return static_cast<double>(*t9_tool_complete_ns - *t8_tool_start_ns) / 1e6;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<double> tool_to_verification_ms() const {
        if (t9_tool_complete_ns && t10_verification_complete_ns && *t10_verification_complete_ns >= *t9_tool_complete_ns) {
            return static_cast<double>(*t10_verification_complete_ns - *t9_tool_complete_ns) / 1e6;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<double> tts_to_first_audio_ms() const {
        return tts_ttfa_ms();
    }

    [[nodiscard]] std::optional<double> complete_end_to_end_ms() const {
        uint64_t start_ns = 0;
        if (t0_audio_capture_ns) start_ns = *t0_audio_capture_ns;
        else if (t1_vad_speech_start_ns) start_ns = *t1_vad_speech_start_ns;
        else if (t2_wake_word_detected_ns) start_ns = *t2_wake_word_detected_ns;
        else return std::nullopt;

        uint64_t end_ns = 0;
        if (t13_tts_complete_ns) end_ns = *t13_tts_complete_ns;
        else if (t10_verification_complete_ns) end_ns = *t10_verification_complete_ns;
        else if (t9_tool_complete_ns) end_ns = *t9_tool_complete_ns;
        else if (t6_intent_detected_ns) end_ns = *t6_intent_detected_ns;
        else if (t4_final_stt_result_ns) end_ns = *t4_final_stt_result_ns;
        else return std::nullopt;

        if (end_ns >= start_ns) {
            return static_cast<double>(end_ns - start_ns) / 1e6;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::string format_stage_breakdown() const {
        std::ostringstream ss;
        ss << "=== Voice Turn Telemetry [" << turn_id << "] ===\n";
        auto print_metric = [&](const char* label, const std::optional<double>& val) {
            ss << "  " << std::left << std::setw(28) << label << ": ";
            if (val.has_value()) {
                ss << std::fixed << std::setprecision(2) << *val << " ms\n";
            } else {
                ss << "N/A (stage inactive)\n";
            }
        };

        print_metric("Audio -> First Partial (T0->T3)", audio_to_first_partial_ms());
        print_metric("Audio -> Final Transcript (T0->T4)", audio_to_final_transcript_ms());
        print_metric("Transcript -> Intent (T4->T6)", transcript_to_intent_ms());
        print_metric("Intent -> Tool Start (T6->T8)", intent_to_tool_start_ms());
        print_metric("Tool Duration (T8->T9)", tool_duration_ms());
        print_metric("Tool -> Verification (T9->T10)", tool_to_verification_ms());
        print_metric("TTS -> First Audio (T11->T12)", tts_to_first_audio_ms());
        print_metric("Complete End-to-End Latency", complete_end_to_end_ms());
        return ss.str();
    }
};

class VoiceTelemetryCollector {
public:
    VoiceTelemetryCollector() = default;

    void record_turn(const VoiceTurnTimestamps& turn) {
        std::lock_guard<std::mutex> lock(mutex_);
        turns_.push_back(turn);
    }

    [[nodiscard]] size_t total_turns_recorded() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return turns_.size();
    }

    [[nodiscard]] std::vector<VoiceTurnTimestamps> all_turns() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return turns_;
    }

    [[nodiscard]] double average_turn_latency_ms() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (turns_.empty()) return 0.0;
        double sum = 0.0;
        size_t count = 0;
        for (const auto& t : turns_) {
            auto e2e = t.complete_end_to_end_ms();
            if (e2e.has_value()) {
                sum += *e2e;
                count++;
            }
        }
        return count > 0 ? (sum / static_cast<double>(count)) : 0.0;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        turns_.clear();
    }

private:
    std::vector<VoiceTurnTimestamps> turns_;
    mutable std::mutex mutex_;
};

} // namespace vani::voice::telemetry
