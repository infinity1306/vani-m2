#include "network_manager.hpp"

namespace vani::capabilities::system {

NetworkManager::NetworkManager(adapters::system::SystemAdapterPtr adapter)
    : adapter_(std::move(adapter)) {}

contracts::Result<NetworkState> NetworkManager::get_network_state() {
    if (!adapter_) {
        return contracts::Result<NetworkState>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    auto state_res = adapter_->get_system_state();
    NetworkState net;
    if (state_res.is_success()) {
        net.is_online = state_res.value().is_online;
    }
    return contracts::Result<NetworkState>::success(net);
}

contracts::Result<void> NetworkManager::modify_network_state(const std::string& /*config*/) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return contracts::Result<void>::failure(
        contracts::ErrorCode::PermissionDenied, "Modifying network configuration requires administrative confirmation"
    );
}

} // namespace vani::capabilities::system
