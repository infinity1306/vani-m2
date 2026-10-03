#pragma once

#include "../../contracts/memory/memory_provider.hpp"
#include <mutex>
#include <vector>
#include <unordered_map>
#include <string>

namespace vani::adapters::memory {

class PersistentMemoryProvider : public contracts::MemoryProvider {
public:
    explicit PersistentMemoryProvider(const std::string& storage_path = "storage/memory/memory_store.json");
    ~PersistentMemoryProvider() override = default;

    contracts::Result<void> store(const contracts::MemoryItem& item) override;
    contracts::Result<std::vector<contracts::MemoryItem>> query(const contracts::MemoryQuery& query) override;
    contracts::Result<void> forget(const std::string& memory_id) override;
    contracts::Result<void> clear_tier(contracts::MemoryTier tier) override;

    [[nodiscard]] std::vector<contracts::MemoryItem> list_all() const;
    [[nodiscard]] size_t count() const noexcept;

private:
    void load_from_disk();
    void save_to_disk();

    mutable std::mutex mutex_;
    std::string storage_path_;
    std::unordered_map<std::string, contracts::MemoryItem> items_;
    uint64_t next_id_seq_{1};
};

using PersistentMemoryProviderPtr = std::shared_ptr<PersistentMemoryProvider>;

} // namespace vani::adapters::memory
