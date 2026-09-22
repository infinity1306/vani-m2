#pragma once

#include "normalized_text.hpp"
#include "../../contracts/providers/stt_engine.hpp"
#include "../../contracts/common/result.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <regex>

namespace vani::voice::normalization {

class LanguageNormalizer {
public:
    LanguageNormalizer();
    ~LanguageNormalizer() = default;

    contracts::Result<NormalizedText> normalize(
        const contracts::STTTranscript& transcript,
        const NormalizationContext& context = {}
    );

    void add_deterministic_rule(const std::string& pattern, const std::string& replacement);

private:
    std::string apply_layer1_deterministic(
        const std::string& input,
        std::vector<TextCorrection>& out_corrections
    );

    std::string apply_layer2_vocabulary(
        const std::string& input,
        const NormalizationContext& context,
        std::vector<TextCorrection>& out_corrections
    );

    std::vector<std::pair<std::regex, std::string>> deterministic_rules_;
};

} // namespace vani::voice::normalization
