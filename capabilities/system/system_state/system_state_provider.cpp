#include "system_state_provider.hpp"

namespace vani::capabilities::system {

SystemStateProvider::SystemStateProvider(adapters::system::SystemAdapterPtr adapter)
    : adapter_(std::move(adapter)) {}

contracts::Result<contracts::SystemStateSnapshot> SystemStateProvider::get_state() {
    if (!adapter_) {
        return contracts::Result<contracts::SystemStateSnapshot>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->get_system_state();
}

} // namespace vani::capabilities::system
