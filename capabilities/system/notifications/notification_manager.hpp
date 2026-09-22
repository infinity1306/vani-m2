#pragma once

#include "../../../adapters/system/system_adapter.hpp"
#include "../../../contracts/common/result.hpp"
#include <memory>
#include <vector>
#include <mutex>

namespace vani::capabilities::system {

class NotificationManager {
public:
    explicit NotificationManager(adapters::system::SystemAdapterPtr adapter);
    ~NotificationManager() = default;

    contracts::Result<void> send_notification(const contracts::NotificationPayload& notification);
    [[nodiscard]] std::vector<contracts::NotificationPayload> list_notification_history() const;

private:
    adapters::system::SystemAdapterPtr adapter_;
    mutable std::mutex mutex_;
    std::vector<contracts::NotificationPayload> history_;
};

using NotificationManagerPtr = std::shared_ptr<NotificationManager>;

} // namespace vani::capabilities::system
