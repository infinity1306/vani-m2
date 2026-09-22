#include "vocabulary_engine.hpp"
#include <algorithm>

namespace vani::voice::vocabulary {

static std::string to_lower(const std::string& str) {
    std::string res = str;
    std::transform(res.begin(), res.end(), res.begin(), ::tolower);
    return res;
}

VocabularyEngine::VocabularyEngine() {
    init_default_vocabulary();
}

void VocabularyEngine::init_default_vocabulary() {
    // Coding scope vocabulary
    add_item({
        .canonical_name = "React",
        .aliases = {"react", "reactjs", "react js"},
        .category = VocabularyCategory::Technology,
        .base_priority = 2.0f,
        .scope = "coding"
    });

    add_item({
        .canonical_name = "TypeScript",
        .aliases = {"typescript", "type script", "ts"},
        .category = VocabularyCategory::Technology,
        .base_priority = 2.0f,
        .scope = "coding"
    });

    add_item({
        .canonical_name = "GitHub",
        .aliases = {"github", "git hub", "git"},
        .category = VocabularyCategory::Application,
        .base_priority = 2.0f,
        .scope = "coding"
    });

    add_item({
        .canonical_name = "Odysseus",
        .aliases = {"odysus", "odysseus ai", "coding agent"},
        .category = VocabularyCategory::Project,
        .base_priority = 3.0f,
        .scope = "general"
    });

    add_item({
        .canonical_name = "Hermes",
        .aliases = {"hermes", "research agent"},
        .category = VocabularyCategory::Project,
        .base_priority = 3.0f,
        .scope = "general"
    });

    add_item({
        .canonical_name = "Google Chrome",
        .aliases = {"chrome", "google chrome", "browser"},
        .category = VocabularyCategory::Application,
        .base_priority = 1.5f,
        .scope = "general"
    });

    add_item({
        .canonical_name = "Visual Studio Code",
        .aliases = {"vs code", "vscode", "code editor"},
        .category = VocabularyCategory::Application,
        .base_priority = 2.0f,
        .scope = "coding"
    });
}

void VocabularyEngine::add_item(const VocabularyItem& item) {
    if (item.canonical_name.empty()) return;

    std::lock_guard<std::mutex> lock(mutex_);
    items_by_name_[item.canonical_name] = item;

    alias_to_canonical_[to_lower(item.canonical_name)] = item.canonical_name;
    for (const auto& alias : item.aliases) {
        alias_to_canonical_[to_lower(alias)] = item.canonical_name;
    }
    for (const auto& phone : item.phonetic_variants) {
        alias_to_canonical_[to_lower(phone)] = item.canonical_name;
    }
}

void VocabularyEngine::remove_item(const std::string& canonical_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = items_by_name_.find(canonical_name);
    if (it != items_by_name_.end()) {
        alias_to_canonical_.erase(to_lower(it->second.canonical_name));
        for (const auto& a : it->second.aliases) {
            alias_to_canonical_.erase(to_lower(a));
        }
        for (const auto& p : it->second.phonetic_variants) {
            alias_to_canonical_.erase(to_lower(p));
        }
        items_by_name_.erase(it);
    }
}

std::optional<VocabularyItem> VocabularyEngine::lookup(const std::string& alias_or_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = alias_to_canonical_.find(to_lower(alias_or_name));
    if (it != alias_to_canonical_.end()) {
        auto item_it = items_by_name_.find(it->second);
        if (item_it != items_by_name_.end()) {
            return item_it->second;
        }
    }
    return std::nullopt;
}

std::vector<VocabularyItem> VocabularyEngine::get_contextual_vocabulary(
    const std::string& active_scope,
    size_t max_items
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<float, VocabularyItem>> scored;

    for (const auto& [name, item] : items_by_name_) {
        float score = item.base_priority;
        if (!active_scope.empty() && (item.scope == active_scope || item.scope == "general")) {
            score *= 2.0f; // Contextual relevance boost
        }
        scored.emplace_back(score, item);
    }

    std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) {
        return a.first > b.first;
    });

    std::vector<VocabularyItem> result;
    for (size_t i = 0; i < std::min(max_items, scored.size()); ++i) {
        result.push_back(scored[i].second);
    }
    return result;
}

size_t VocabularyEngine::total_items() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return items_by_name_.size();
}

} // namespace vani::voice::vocabulary
