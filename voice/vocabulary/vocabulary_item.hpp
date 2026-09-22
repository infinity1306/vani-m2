#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace vani::voice::vocabulary {

enum class VocabularyCategory : uint8_t {
    Project,
    Application,
    Person,
    Company,
    Technology,
    Repository,
    Command,
    Custom
};

struct VocabularyItem {
    std::string canonical_name;
    std::vector<std::string> aliases;
    std::vector<std::string> phonetic_variants;
    VocabularyCategory category{VocabularyCategory::Custom};
    float base_priority{1.0f};
    float confidence{1.0f};
    std::string scope; // e.g. "coding", "crm", "general"
    std::string source{"system"};
};

} // namespace vani::voice::vocabulary
