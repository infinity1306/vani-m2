#include "language_normalizer.hpp"
#include "../language/language_detector.hpp"

namespace vani::voice::normalization {

LanguageNormalizer::LanguageNormalizer() {
    // Layer 1 Standard Canonical Corrections
    add_deterministic_rule(R"(\bodysus\b)", "Odysseus");
    add_deterministic_rule(R"(\bodi(ss|s)eus\b)", "Odysseus");
    add_deterministic_rule(R"(\bvani\b)", "VANI");
    add_deterministic_rule(R"(\bvan i\b)", "VANI");
    add_deterministic_rule(R"(\breact js\b)", "React.js");
    add_deterministic_rule(R"(\bgit hub\b)", "GitHub");
    add_deterministic_rule(R"(\bvs code\b)", "VS Code");
    add_deterministic_rule(R"(\btype script\b)", "TypeScript");
    add_deterministic_rule(R"(\bchat gpt\b)", "ChatGPT");
    add_deterministic_rule(R"(\bclaude\b)", "Claude");
    add_deterministic_rule(R"(\bhermes\b)", "Hermes");
    add_deterministic_rule(R"(\brum karde\b)", "run kar de");
    add_deterministic_rule(R"(\brun karde\b)", "run kar de");
    add_deterministic_rule(R"(\bchalu kardo\b)", "start kar do");
}

void LanguageNormalizer::add_deterministic_rule(
    const std::string& pattern,
    const std::string& replacement
) {
    deterministic_rules_.emplace_back(
        std::regex(pattern, std::regex_constants::icase),
        replacement
    );
}

std::string LanguageNormalizer::apply_layer1_deterministic(
    const std::string& input,
    std::vector<TextCorrection>& out_corrections
) {
    std::string result = input;

    for (const auto& [reg, replacement] : deterministic_rules_) {
        std::smatch match;
        if (std::regex_search(result, match, reg)) {
            out_corrections.push_back({
                .original_span = match.str(),
                .corrected_span = replacement,
                .start_pos = static_cast<size_t>(match.position()),
                .length = static_cast<size_t>(match.length()),
                .rule_or_source = "Layer1:Deterministic"
            });
            result = std::regex_replace(result, reg, replacement);
        }
    }

    return result;
}

std::string LanguageNormalizer::apply_layer2_vocabulary(
    const std::string& input,
    const NormalizationContext& context,
    std::vector<TextCorrection>& out_corrections
) {
    std::string result = input;

    for (const auto& term : context.contextual_terms) {
        if (term.empty()) continue;

        std::string pattern = "\\b" + term + "\\b";
        try {
            std::regex term_reg(pattern, std::regex_constants::icase);
            std::smatch match;
            if (std::regex_search(result, match, term_reg) && match.str() != term) {
                out_corrections.push_back({
                    .original_span = match.str(),
                    .corrected_span = term,
                    .start_pos = static_cast<size_t>(match.position()),
                    .length = static_cast<size_t>(match.length()),
                    .rule_or_source = "Layer2:ContextualVocabulary"
                });
                result = std::regex_replace(result, term_reg, term);
            }
        } catch (...) {}
    }

    return result;
}

contracts::Result<NormalizedText> LanguageNormalizer::normalize(
    const contracts::STTTranscript& transcript,
    const NormalizationContext& context
) {
    if (transcript.text.empty()) {
        return contracts::Ok(NormalizedText{
            .raw_text = "",
            .normalized_text = "",
            .language = "unknown",
            .confidence = 1.0f,
            .corrections = {},
            .layer_applied = 0
        });
    }

    std::vector<TextCorrection> corrections;
    std::string l1_text = apply_layer1_deterministic(transcript.text, corrections);
    std::string final_text = apply_layer2_vocabulary(l1_text, context, corrections);

    auto detected_lang = language::LanguageDetector::detect(final_text);

    return contracts::Ok(NormalizedText{
        .raw_text = transcript.text,
        .normalized_text = final_text,
        .language = detected_lang.iso_code,
        .confidence = transcript.confidence,
        .corrections = std::move(corrections),
        .layer_applied = 2
    });
}

} // namespace vani::voice::normalization
