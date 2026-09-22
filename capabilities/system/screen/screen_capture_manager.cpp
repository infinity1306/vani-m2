#include "screen_capture_manager.hpp"

namespace vani::capabilities::system {

ScreenCaptureManager::ScreenCaptureManager(adapters::system::SystemAdapterPtr adapter)
    : adapter_(std::move(adapter)) {}

contracts::Result<contracts::ScreenCaptureMetadata> ScreenCaptureManager::capture_screen(
    uint32_t monitor_id,
    uint32_t window_id
) {
    if (!adapter_) {
        return contracts::Result<contracts::ScreenCaptureMetadata>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->capture_screen(monitor_id, window_id);
}

} // namespace vani::capabilities::system
