#pragma once

#include "../../../contracts/common/result.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <optional>

namespace vani::capabilities::system {

enum class FileOperationType : uint8_t {
    Create,
    Modify,
    Delete,
    Rename,
    Move
};

struct FileSnapshot {
    std::string path;
    std::string previous_content;
    bool existed_previously{false};
    std::string target_path; // For rename / move
    std::string trash_location;
};

struct FileTransaction {
    std::string transaction_id;
    std::string task_id;
    uint64_t created_at_ms{0};
    std::vector<FileSnapshot> snapshots;
    bool is_committed{false};
    bool is_rolled_back{false};
};

class FileTransactionManager {
public:
    FileTransactionManager() = default;
    ~FileTransactionManager() = default;

    std::string begin_transaction(const std::string& task_id);
    void record_snapshot(const std::string& transaction_id, FileSnapshot snapshot);
    contracts::Result<void> commit(const std::string& transaction_id);
    [[nodiscard]] std::optional<FileTransaction> get_transaction(const std::string& transaction_id) const;
    [[nodiscard]] std::vector<FileTransaction> list_transactions() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, FileTransaction> transactions_;
    uint64_t next_tx_seq_{1};
};

using FileTransactionManagerPtr = std::shared_ptr<FileTransactionManager>;

} // namespace vani::capabilities::system
