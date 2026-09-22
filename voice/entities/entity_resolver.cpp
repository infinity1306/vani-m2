#include "entity_resolver.hpp"
#include <regex>
#include <algorithm>

namespace vani::voice::entities {

EntityResolver::EntityResolver(vocabulary::VocabularyEnginePtr vocab_engine)
    : vocab_engine_(std::move(vocab_engine)) {}

static EntityType to_entity_type(vocabulary::VocabularyCategory cat) {
    switch (cat) {
        case vocabulary::VocabularyCategory::Application: return EntityType::Application;
        case vocabulary::VocabularyCategory::Project:     return EntityType::Project;
        case vocabulary::VocabularyCategory::Technology:  return EntityType::Technology;
        case vocabulary::VocabularyCategory::Person:      return EntityType::Contact;
        default:                                          return EntityType::Custom;
    }
}

std::vector<ResolvedEntity> EntityResolver::resolve_entities(
    const std::string& normalized_text,
    const std::string& active_scope
) {
    if (!vocab_engine_ || normalized_text.empty()) return {};

    std::vector<ResolvedEntity> resolved;
    auto vocab_items = vocab_engine_->get_contextual_vocabulary(active_scope, 100);

    for (const auto& item : vocab_items) {
        // Check canonical name
        std::string pattern = "\\b" + item.canonical_name + "\\b";
        try {
            std::regex reg(pattern, std::regex_constants::icase);
            std::smatch match;
            if (std::regex_search(normalized_text, match, reg)) {
                resolved.push_back({
                    .raw_span = match.str(),
                    .canonical_name = item.canonical_name,
                    .type = to_entity_type(item.category),
                    .confidence = item.confidence,
                    .is_ambiguous = false,
                    .candidate_alternatives = {}
                });
                continue;
            }

            // Check aliases
            for (const auto& alias : item.aliases) {
                std::string alias_pattern = "\\b" + alias + "\\b";
                std::regex alias_reg(alias_pattern, std::regex_constants::icase);
                if (std::regex_search(normalized_text, match, alias_reg)) {
                    resolved.push_back({
                        .raw_span = match.str(),
                        .canonical_name = item.canonical_name,
                        .type = to_entity_type(item.category),
                        .confidence = item.confidence * 0.95f,
                        .is_ambiguous = false,
                        .candidate_alternatives = {}
                    });
                    break;
                }
            }
        } catch (...) {}
    }

    return resolved;
}

} // namespace vani::voice::entities
