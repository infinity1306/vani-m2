#pragma once

#include "../entities/entity_resolver.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace vani::voice::intent {

struct ConfidenceVector {
    float audio_confidence{1.0f};
    float stt_confidence{0.95f};
    float language_confidence{0.90f};
    float normalization_confidence{1.0f};
    float entity_confidence{1.0f};
    float intent_confidence{0.85f};

    [[nodiscard]] float overall_confidence() const noexcept {
        return (audio_confidence * 0.15f) +
               (stt_confidence * 0.35f) +
               (language_confidence * 0.10f) +
               (normalization_confidence * 0.15f) +
               (entity_confidence * 0.15f) +
               (intent_confidence * 0.10f);
    }
};

struct CandidateIntent {
    std::string intent_name;
    float score{0.0f};
    std::string suggested_agent; // e.g. "odysseus", "hermes"
    std::vector<std::string> required_tools;
};

struct Utterance {
    std::string session_id;
    uint64_t timestamp_ms{0};
    std::string raw_transcript;
    std::string normalized_transcript;
    std::string language;
    ConfidenceVector confidence;
    std::vector<entities::ResolvedEntity> entities;
    std::vector<CandidateIntent> candidate_intents;
    std::string preferred_route;
};

} // namespace vani::voice::intent
