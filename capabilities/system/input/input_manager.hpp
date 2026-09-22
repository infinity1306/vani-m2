#pragma once

#include "../../../adapters/system/system_adapter.hpp"
#include "../../../contracts/common/result.hpp"
#include <memory>
#include <string>

namespace vani::capabilities::system {

class InputManager {
public:
    explicit InputManager(adapters::system::SystemAdapterPtr adapter);
    ~InputManager() = default;

    contracts::Result<void> press_key(
        const std::string& key,
        const contracts::InputTargetContext& target = {}
    );
    contracts::Result<void> type_text(
        const std::string& text,
        const contracts::InputTargetContext& target = {}
    );
    contracts::Result<void> click(
        contracts::MouseButton button,
        int32_t x,
        int32_t y,
        const contracts::InputTargetContext& target = {}
    );
    contracts::Result<void> move_mouse(
        int32_t x,
        int32_t y,
        const contracts::InputTargetContext& target = {}
    );
    contracts::Result<void> scroll_mouse(
        int32_t delta_y,
        const contracts::InputTargetContext& target = {}
    );

private:
    adapters::system::SystemAdapterPtr adapter_;
};

using InputManagerPtr = std::shared_ptr<InputManager>;

} // namespace vani::capabilities::system
