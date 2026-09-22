#pragma once

#include "../common/version.hpp"
#include "../common/result.hpp"
#include <string>
#include <vector>
#include <memory>

namespace vani::contracts {

enum class MemoryTier : uint8_t {
    Session,
    Working,
    Semantic,
    Episodic,
    Project,
    Preferences,
    Vocabulary
};

struct MemoryItem {
    std::string id;
    MemoryTier tier{MemoryTier::Working};
    std::string key;
    std::string content;
    std::string source;
    float confidence{1.0f};
    uint64_t created_at_ms{0};
    uint64_t last_accessed_ms{0};
    bool is_pinned{false};
    std::string transparency_reason;
};

struct MemoryQuery {
    std::string text_query;
    std::vector<MemoryTier> allowed_tiers;
    uint32_t max_results{10};
    float min_relevance{0.6f};
};

class MemoryProvider {
public:
    virtual ~MemoryProvider() = default;

    virtual Result<void> store(const MemoryItem& item) = 0;

    virtual Result<std::vector<MemoryItem>> query(const MemoryQuery& query) = 0;

    virtual Result<void> forget(const std::string& memory_id) = 0;

    virtual Result<void> clear_tier(MemoryTier tier) = 0;
};

using MemoryProviderPtr = std::shared_ptr<MemoryProvider>;

} // namespace vani::contracts
