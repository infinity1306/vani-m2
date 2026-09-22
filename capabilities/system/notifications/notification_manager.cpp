#include "notification_manager.hpp"

namespace vani::capabilities::system {

NotificationManager::NotificationManager(adapters::system::SystemAdapterPtr adapter)
    : adapter_(std::move(adapter)) {}

contracts::Result<void> NotificationManager::send_notification(const contracts::NotificationPayload& notification) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        history_.push_back(notification);
    }

    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->send_notification(notification);
}

std::vector<contracts::NotificationPayload> NotificationManager::list_notification_history() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return history_;
}

} // namespace vani::capabilities::system
