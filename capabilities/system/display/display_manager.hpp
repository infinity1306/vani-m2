#pragma once

#include "../../../adapters/system/system_adapter.hpp"
#include "../../../contracts/common/result.hpp"
#include <memory>
#include <vector>

namespace vani::capabilities::system {

class DisplayManager {
public:
    explicit DisplayManager(adapters::system::SystemAdapterPtr adapter);
    ~DisplayManager() = default;

    contracts::Result<std::vector<contracts::DisplayMetadata>> list_displays();
    contracts::Result<uint32_t> get_brightness(uint32_t display_id = 0);
    contracts::Result<void> set_brightness(uint32_t display_id, uint32_t percent);

private:
    adapters::system::SystemAdapterPtr adapter_;
};

using DisplayManagerPtr = std::shared_ptr<DisplayManager>;

} // namespace vani::capabilities::system
