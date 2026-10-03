#include "undo_manager.hpp"
#include <chrono>

namespace vani::capabilities::system {

static uint64_t current_time_ms() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

UndoManager::UndoManager(adapters::system::SystemAdapterPtr adapter)
    : adapter_(std::move(adapter)) {}

void UndoManager::record_action(const UndoAction& action) {
    std::lock_guard<std::mutex> lock(mutex_);
    UndoAction act = action;
    if (act.id.empty()) {
        act.id = "undo_" + std::to_string(current_time_ms()) + "_" + std::to_string(next_id_++);
    }
    if (act.timestamp_ms == 0) {
        act.timestamp_ms = current_time_ms();
    }
    history_.push_back(act);
}

contracts::Result<UndoAction> UndoManager::undo_last() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (history_.empty()) {
        return contracts::Result<UndoAction>::failure(
            contracts::ErrorCode::NotFound, "No actions available to undo"
        );
    }

    if (!adapter_) {
        return contracts::Result<UndoAction>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }

    UndoAction action = history_.back();
    history_.pop_back();

    switch (action.type) {
        case UndoActionType::FileWrite: {
            if (action.target_existed) {
                auto res = adapter_->write_file(action.target_path, action.previous_data);
                if (!res.is_success()) return contracts::Result<UndoAction>::failure(res.error());
            } else {
                auto res = adapter_->delete_file(action.target_path, false);
                if (!res.is_success()) return contracts::Result<UndoAction>::failure(res.error());
            }
            break;
        }
        case UndoActionType::FileDelete: {
            auto res = adapter_->write_file(action.target_path, action.previous_data);
            if (!res.is_success()) return contracts::Result<UndoAction>::failure(res.error());
            break;
        }
        case UndoActionType::FileMove:
        case UndoActionType::FileRename: {
            auto res = adapter_->move_file(action.new_path, action.target_path);
            if (!res.is_success()) return contracts::Result<UndoAction>::failure(res.error());
            break;
        }
        case UndoActionType::ClipboardWrite: {
            contracts::ClipboardPayload payload;
            payload.text_content = action.previous_data;
            auto res = adapter_->write_clipboard(payload);
            if (!res.is_success()) return contracts::Result<UndoAction>::failure(res.error());
            break;
        }
    }

    return contracts::Result<UndoAction>::success(action);
}

std::vector<UndoAction> UndoManager::list_history() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return history_;
}

size_t UndoManager::history_count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return history_.size();
}

void UndoManager::clear_history() {
    std::lock_guard<std::mutex> lock(mutex_);
    history_.clear();
}

} // namespace vani::capabilities::system
