#pragma once

#include "../../../adapters/system/system_adapter.hpp"
#include "../../../contracts/common/result.hpp"
#include <memory>

namespace vani::capabilities::system {

class PowerManager {
public:
    explicit PowerManager(adapters::system::SystemAdapterPtr adapter);
    ~PowerManager() = default;

    contracts::Result<void> lock_system();
    contracts::Result<void> sleep_system();
    contracts::Result<void> restart_system(bool user_confirmed = false);
    contracts::Result<void> shutdown_system(bool user_confirmed = false);

private:
    adapters::system::SystemAdapterPtr adapter_;
};

using PowerManagerPtr = std::shared_ptr<PowerManager>;

} // namespace vani::capabilities::system
