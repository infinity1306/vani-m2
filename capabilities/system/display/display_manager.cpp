#include "display_manager.hpp"

namespace vani::capabilities::system {

DisplayManager::DisplayManager(adapters::system::SystemAdapterPtr adapter)
    : adapter_(std::move(adapter)) {}

contracts::Result<std::vector<contracts::DisplayMetadata>> DisplayManager::list_displays() {
    if (!adapter_) {
        return contracts::Result<std::vector<contracts::DisplayMetadata>>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->list_displays();
}

contracts::Result<uint32_t> DisplayManager::get_brightness(uint32_t display_id) {
    if (!adapter_) {
        return contracts::Result<uint32_t>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->get_brightness(display_id);
}

contracts::Result<void> DisplayManager::set_brightness(uint32_t display_id, uint32_t percent) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->set_brightness(display_id, percent);
}

} // namespace vani::capabilities::system
