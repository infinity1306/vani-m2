#pragma once

#include "../../../contracts/system/system_contracts.hpp"
#include <vector>
#include <string>
#include <mutex>
#include <memory>

namespace vani::capabilities::system {

class SystemActionJournal {
public:
    SystemActionJournal() = default;
    ~SystemActionJournal() = default;

    void log_action(contracts::SystemActionJournalEntry entry);
    [[nodiscard]] std::vector<contracts::SystemActionJournalEntry> list_entries(size_t limit = 100) const;
    [[nodiscard]] std::vector<contracts::SystemActionJournalEntry> list_by_task(const std::string& task_id) const;
    [[nodiscard]] size_t count() const noexcept;
    void clear();

private:
    mutable std::mutex mutex_;
    std::vector<contracts::SystemActionJournalEntry> entries_;
    uint64_t next_seq_{1};
};

using SystemActionJournalPtr = std::shared_ptr<SystemActionJournal>;

} // namespace vani::capabilities::system
