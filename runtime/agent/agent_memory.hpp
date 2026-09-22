#pragma once

#include "../../contracts/agents/agent_execution_contracts.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <memory>

namespace vani::runtime::agent {

struct MemoryEntry {
    std::string key;
    std::string content;
    std::string source_step_id;
    uint64_t timestamp_ms{0};
};

class ShortTermTaskMemory {
public:
    explicit ShortTermTaskMemory(uint32_t max_tokens = 4096);
    ~ShortTermTaskMemory() = default;

    void add_observation(const contracts::AgentObservation& obs);
    void add_entry(const std::string& key, const std::string& content, const std::string& step_id = "");

    [[nodiscard]] std::vector<MemoryEntry> entries() const;
    [[nodiscard]] size_t entry_count() const;
    [[nodiscard]] uint32_t estimated_tokens() const;

    // Compacts old entries if estimated tokens exceed max_tokens
    void compact_if_needed();

    // Redacts secrets/credentials
    [[nodiscard]] static std::string scrub_sensitive_data(const std::string& text);

    void clear();

private:
    mutable std::mutex mutex_;
    uint32_t max_tokens_{4096};
    std::vector<MemoryEntry> entries_;
};

using ShortTermTaskMemoryPtr = std::shared_ptr<ShortTermTaskMemory>;

} // namespace vani::runtime::agent
