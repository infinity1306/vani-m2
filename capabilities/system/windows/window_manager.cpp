#include "window_manager.hpp"

namespace vani::capabilities::system {

WindowManager::WindowManager(adapters::system::SystemAdapterPtr adapter)
    : adapter_(std::move(adapter)) {}

contracts::Result<std::vector<contracts::WindowMetadata>> WindowManager::list_windows() {
    if (!adapter_) {
        return contracts::Result<std::vector<contracts::WindowMetadata>>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->list_windows();
}

contracts::Result<void> WindowManager::focus_window(uint32_t window_id) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->focus_window(window_id);
}

contracts::Result<void> WindowManager::minimize_window(uint32_t window_id) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->set_window_state(window_id, contracts::WindowState::Minimized);
}

contracts::Result<void> WindowManager::maximize_window(uint32_t window_id) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->set_window_state(window_id, contracts::WindowState::Maximized);
}

contracts::Result<void> WindowManager::restore_window(uint32_t window_id) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->set_window_state(window_id, contracts::WindowState::Normal);
}

contracts::Result<void> WindowManager::resize_window(uint32_t window_id, const contracts::WindowRect& rect) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    if (rect.width == 0 || rect.height == 0) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::ValidationError, "Window width and height must be positive"
        );
    }
    return adapter_->resize_window(window_id, rect);
}

contracts::Result<void> WindowManager::close_window(uint32_t window_id) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->close_window(window_id);
}

} // namespace vani::capabilities::system
