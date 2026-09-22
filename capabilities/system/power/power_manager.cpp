#include "power_manager.hpp"

namespace vani::capabilities::system {

PowerManager::PowerManager(adapters::system::SystemAdapterPtr adapter)
    : adapter_(std::move(adapter)) {}

contracts::Result<void> PowerManager::lock_system() {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->execute_power_action(contracts::PowerAction::Lock);
}

contracts::Result<void> PowerManager::sleep_system() {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->execute_power_action(contracts::PowerAction::Sleep);
}

contracts::Result<void> PowerManager::restart_system(bool user_confirmed) {
    if (!user_confirmed) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::PermissionDenied, "System restart requires explicit user confirmation"
        );
    }
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->execute_power_action(contracts::PowerAction::Restart);
}

contracts::Result<void> PowerManager::shutdown_system(bool user_confirmed) {
    if (!user_confirmed) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::PermissionDenied, "System shutdown requires explicit user confirmation"
        );
    }
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->execute_power_action(contracts::PowerAction::Shutdown);
}

} // namespace vani::capabilities::system
