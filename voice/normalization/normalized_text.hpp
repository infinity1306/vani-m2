#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace vani::voice::normalization {

struct TextCorrection {
    std::string original_span;
    std::string corrected_span;
    size_t start_pos{0};
    size_t length{0};
    std::string rule_or_source;
};

struct NormalizedText {
    std::string raw_text;
    std::string normalized_text;
    std::string language{"unknown"};
    float confidence{1.0f};
    std::vector<TextCorrection> corrections;
    uint32_t layer_applied{0}; // 1 = deterministic, 2 = vocabulary, 3 = semantic
};

struct NormalizationContext {
    std::string active_app;
    std::string active_project;
    std::vector<std::string> contextual_terms;
    bool enable_hinglish{true};
};

} // namespace vani::voice::normalization
