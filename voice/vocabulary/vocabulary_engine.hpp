#pragma once

#include "vocabulary_item.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <optional>

namespace vani::voice::vocabulary {

class VocabularyEngine {
public:
    VocabularyEngine();
    ~VocabularyEngine() = default;

    void add_item(const VocabularyItem& item);
    void remove_item(const std::string& canonical_name);

    [[nodiscard]] std::optional<VocabularyItem> lookup(const std::string& alias_or_name) const;

    [[nodiscard]] std::vector<VocabularyItem> get_contextual_vocabulary(
        const std::string& active_scope,
        size_t max_items = 50
    ) const;

    [[nodiscard]] size_t total_items() const;

private:
    void init_default_vocabulary();

    std::unordered_map<std::string, VocabularyItem> items_by_name_;
    std::unordered_map<std::string, std::string> alias_to_canonical_;
    mutable std::mutex mutex_;
};

using VocabularyEnginePtr = std::shared_ptr<VocabularyEngine>;

} // namespace vani::voice::vocabulary
