#include "input_manager.hpp"

namespace vani::capabilities::system {

InputManager::InputManager(adapters::system::SystemAdapterPtr adapter)
    : adapter_(std::move(adapter)) {}

contracts::Result<void> InputManager::press_key(
    const std::string& key,
    const contracts::InputTargetContext& target
) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->inject_key_press(key, target);
}

contracts::Result<void> InputManager::type_text(
    const std::string& text,
    const contracts::InputTargetContext& target
) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->inject_key_sequence(text, target);
}

contracts::Result<void> InputManager::click(
    contracts::MouseButton button,
    int32_t x,
    int32_t y,
    const contracts::InputTargetContext& target
) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->inject_mouse_click(button, x, y, target);
}

contracts::Result<void> InputManager::move_mouse(
    int32_t x,
    int32_t y,
    const contracts::InputTargetContext& target
) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->inject_mouse_move(x, y, target);
}

contracts::Result<void> InputManager::scroll_mouse(
    int32_t delta_y,
    const contracts::InputTargetContext& target
) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->inject_mouse_scroll(delta_y, target);
}

} // namespace vani::capabilities::system
