#pragma once

#include "../../../contracts/common/result.hpp"
#include "../../../adapters/system/system_adapter.hpp"
#include "../filesystem/filesystem_manager.hpp"
#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace vani::capabilities::system {

enum class UndoActionType : uint8_t {
    FileWrite,
    FileDelete,
    FileMove,
    FileRename,
    ClipboardWrite
};

struct UndoAction {
    std::string id;
    UndoActionType type;
    std::string target_path;
    std::string new_path;       // For move/rename
    std::string previous_data;  // Old file content or old clipboard
    bool target_existed{false};
    std::string transaction_id;
    uint64_t timestamp_ms{0};
    std::string description;
};

class UndoManager {
public:
    explicit UndoManager(adapters::system::SystemAdapterPtr adapter);
    ~UndoManager() = default;

    void record_action(const UndoAction& action);

    contracts::Result<UndoAction> undo_last();

    [[nodiscard]] std::vector<UndoAction> list_history() const;
    [[nodiscard]] size_t history_count() const noexcept;
    void clear_history();

private:
    mutable std::mutex mutex_;
    adapters::system::SystemAdapterPtr adapter_;
    std::vector<UndoAction> history_;
    uint64_t next_id_{1};
};

using UndoManagerPtr = std::shared_ptr<UndoManager>;

} // namespace vani::capabilities::system
