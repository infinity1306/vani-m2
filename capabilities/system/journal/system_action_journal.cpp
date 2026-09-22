#include "system_action_journal.hpp"
#include <chrono>

namespace vani::capabilities::system {

void SystemActionJournal::log_action(contracts::SystemActionJournalEntry entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (entry.timestamp_ms == 0) {
        entry.timestamp_ms = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
    }
    if (entry.entry_id.empty()) {
        entry.entry_id = "jnl_" + std::to_string(next_seq_++);
    }
    entries_.push_back(std::move(entry));
}

std::vector<contracts::SystemActionJournalEntry> SystemActionJournal::list_entries(size_t limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (entries_.size() <= limit) {
        return entries_;
    }
    return std::vector<contracts::SystemActionJournalEntry>(
        entries_.end() - static_cast<std::ptrdiff_t>(limit), entries_.end()
    );
}

std::vector<contracts::SystemActionJournalEntry> SystemActionJournal::list_by_task(const std::string& task_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<contracts::SystemActionJournalEntry> filtered;
    for (const auto& entry : entries_) {
        if (entry.task_id == task_id) {
            filtered.push_back(entry);
        }
    }
    return filtered;
}

size_t SystemActionJournal::count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

void SystemActionJournal::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
}

} // namespace vani::capabilities::system
