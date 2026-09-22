#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

namespace vani::voice::evaluation {

enum class VoiceFailureReason : uint8_t {
    None,
    MicCaptureFailure,
    VadFailure,
    SttFailure,
    SttLanguageFailure,
    SttTechnicalVocabularyFailure,
    NormalizationFailure,
    IntentFailure,
    RoutingFailure,
    PolicyFailure,
    ExecutionFailure,
    VerificationFailure,
    Unknown
};

inline std::string failure_reason_to_string(VoiceFailureReason reason) {
    switch (reason) {
        case VoiceFailureReason::None: return "NONE";
        case VoiceFailureReason::MicCaptureFailure: return "MIC_CAPTURE_FAILURE";
        case VoiceFailureReason::VadFailure: return "VAD_FAILURE";
        case VoiceFailureReason::SttFailure: return "STT_FAILURE";
        case VoiceFailureReason::SttLanguageFailure: return "STT_LANGUAGE_FAILURE";
        case VoiceFailureReason::SttTechnicalVocabularyFailure: return "STT_TECHNICAL_VOCABULARY_FAILURE";
        case VoiceFailureReason::NormalizationFailure: return "NORMALIZATION_FAILURE";
        case VoiceFailureReason::IntentFailure: return "INTENT_FAILURE";
        case VoiceFailureReason::RoutingFailure: return "ROUTING_FAILURE";
        case VoiceFailureReason::PolicyFailure: return "POLICY_FAILURE";
        case VoiceFailureReason::ExecutionFailure: return "EXECUTION_FAILURE";
        case VoiceFailureReason::VerificationFailure: return "VERIFICATION_FAILURE";
        case VoiceFailureReason::Unknown: default: return "UNKNOWN";
    }
}

inline VoiceFailureReason classify_voice_failure(
    bool mic_ok, bool vad_ok, bool got_transcript,
    bool stt_accurate, bool norm_transformed,
    bool intent_accurate, const std::string& category
) {
    (void)norm_transformed;
    if (!mic_ok) return VoiceFailureReason::MicCaptureFailure;
    if (!vad_ok) return VoiceFailureReason::VadFailure;
    if (!got_transcript) return VoiceFailureReason::SttFailure;
    if (!stt_accurate) {
        if (category == "Hindi" || category == "Hinglish" || category == "Long Hinglish") {
            return VoiceFailureReason::SttLanguageFailure;
        }
        if (category == "Technical") {
            return VoiceFailureReason::SttTechnicalVocabularyFailure;
        }
        return VoiceFailureReason::SttFailure;
    }
    if (!intent_accurate) return VoiceFailureReason::IntentFailure;
    return VoiceFailureReason::None;
}

inline std::vector<std::string> split_words(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream iss(text);
    std::string w;
    while (iss >> w) {
        // Strip punctuation and convert to lowercase
        std::string cleaned;
        for (char c : w) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                cleaned += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
        }
        if (!cleaned.empty()) {
            words.push_back(cleaned);
        }
    }
    return words;
}

inline double calculate_word_error_rate(const std::string& reference, const std::string& hypothesis) {
    auto ref_words = split_words(reference);
    auto hyp_words = split_words(hypothesis);

    if (ref_words.empty()) {
        return hyp_words.empty() ? 0.0 : 1.0;
    }
    if (hyp_words.empty()) {
        return 1.0;
    }

    const size_t n = ref_words.size();
    const size_t m = hyp_words.size();

    std::vector<std::vector<size_t>> d(n + 1, std::vector<size_t>(m + 1, 0));

    for (size_t i = 0; i <= n; ++i) d[i][0] = i;
    for (size_t j = 0; j <= m; ++j) d[0][j] = j;

    for (size_t i = 1; i <= n; ++i) {
        for (size_t j = 1; j <= m; ++j) {
            if (ref_words[i - 1] == hyp_words[j - 1]) {
                d[i][j] = d[i - 1][j - 1];
            } else {
                size_t sub = d[i - 1][j - 1] + 1;
                size_t ins = d[i][j - 1] + 1;
                size_t del = d[i - 1][j] + 1;
                d[i][j] = std::min({sub, ins, del});
            }
        }
    }

    return static_cast<double>(d[n][m]) / static_cast<double>(n);
}

inline double calculate_percentile(std::vector<double> values, double percentile) {
    if (values.empty()) return 0.0;
    std::sort(values.begin(), values.end());
    if (percentile <= 0.0) return values.front();
    if (percentile >= 100.0) return values.back();

    double index = (percentile / 100.0) * (values.size() - 1);
    size_t lower = static_cast<size_t>(std::floor(index));
    size_t upper = static_cast<size_t>(std::ceil(index));
    double weight = index - lower;

    return values[lower] * (1.0 - weight) + values[upper] * weight;
}

struct UtteranceMetrics {
    std::string test_id;
    std::string category;
    std::string reference_phrase;
    std::string actual_transcript;
    std::string normalized_text;
    std::string detected_intent;
    std::string target_intent;
    bool is_command_accurate{false};
    double wer{0.0};
    VoiceFailureReason failure_reason{VoiceFailureReason::None};

    // Latencies (ms)
    double audio_duration_ms{0.0};
    double t0_to_t3_ms{0.0}; // Processing latency to first partial
    double t0_to_t4_ms{0.0}; // Processing latency to final STT
    double t1_to_t3_ms{0.0}; // Human speech start to first partial
    double t1_to_t4_ms{0.0}; // Human speech start to final STT
    double speech_end_to_t4_ms{0.0}; // Speech end to final STT
    double total_e2e_ms{0.0}; // Total pipeline latency (T0->T6)
    double rtf{0.0};
    double memory_mb{0.0};
};

struct CategorySummary {
    std::string category;
    size_t total_samples{0};
    size_t accurate_commands{0};
    double command_accuracy{0.0};
    double avg_wer{0.0};
    double avg_t0_to_t3_ms{0.0};
    double p50_t0_to_t3_ms{0.0};
    double p95_t0_to_t3_ms{0.0};
    double avg_t0_to_t4_ms{0.0};
    double p50_t0_to_t4_ms{0.0};
    double p95_t0_to_t4_ms{0.0};
    double avg_speech_end_to_t4_ms{0.0};
    double avg_rtf{0.0};
};

} // namespace vani::voice::evaluation
