#pragma once

#include "../../../adapters/system/system_adapter.hpp"
#include "../../../contracts/common/result.hpp"
#include <memory>
#include <vector>
#include <string>

namespace vani::capabilities::system {

class WindowManager {
public:
    explicit WindowManager(adapters::system::SystemAdapterPtr adapter);
    ~WindowManager() = default;

    contracts::Result<std::vector<contracts::WindowMetadata>> list_windows();
    contracts::Result<void> focus_window(uint32_t window_id);
    contracts::Result<void> minimize_window(uint32_t window_id);
    contracts::Result<void> maximize_window(uint32_t window_id);
    contracts::Result<void> restore_window(uint32_t window_id);
    contracts::Result<void> resize_window(uint32_t window_id, const contracts::WindowRect& rect);
    contracts::Result<void> close_window(uint32_t window_id);

private:
    adapters::system::SystemAdapterPtr adapter_;
};

using WindowManagerPtr = std::shared_ptr<WindowManager>;

} // namespace vani::capabilities::system
