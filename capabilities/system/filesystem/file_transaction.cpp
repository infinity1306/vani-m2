#include "file_transaction.hpp"

namespace vani::capabilities::system {

std::string FileTransactionManager::begin_transaction(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string tx_id = "tx_" + std::to_string(next_tx_seq_++);
    uint64_t now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    FileTransaction tx;
    tx.transaction_id = tx_id;
    tx.task_id = task_id;
    tx.created_at_ms = now_ms;
    transactions_[tx_id] = tx;

    return tx_id;
}

void FileTransactionManager::record_snapshot(const std::string& transaction_id, FileSnapshot snapshot) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = transactions_.find(transaction_id);
    if (it != transactions_.end()) {
        it->second.snapshots.push_back(std::move(snapshot));
    }
}

contracts::Result<void> FileTransactionManager::commit(const std::string& transaction_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = transactions_.find(transaction_id);
    if (it != transactions_.end()) {
        it->second.is_committed = true;
        return contracts::Result<void>::success();
    }
    return contracts::Result<void>::failure(
        contracts::ErrorCode::NotFound, "Transaction not found: " + transaction_id
    );
}

std::optional<FileTransaction> FileTransactionManager::get_transaction(const std::string& transaction_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = transactions_.find(transaction_id);
    if (it != transactions_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<FileTransaction> FileTransactionManager::list_transactions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<FileTransaction> list;
    list.reserve(transactions_.size());
    for (const auto& [id, tx] : transactions_) {
        list.push_back(tx);
    }
    return list;
}

} // namespace vani::capabilities::system
