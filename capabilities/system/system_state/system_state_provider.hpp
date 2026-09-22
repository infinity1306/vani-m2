#pragma once

#include "../../../adapters/system/system_adapter.hpp"
#include "../../../contracts/common/result.hpp"
#include <memory>

namespace vani::capabilities::system {

class SystemStateProvider {
public:
    explicit SystemStateProvider(adapters::system::SystemAdapterPtr adapter);
    ~SystemStateProvider() = default;

    contracts::Result<contracts::SystemStateSnapshot> get_state();

private:
    adapters::system::SystemAdapterPtr adapter_;
};

using SystemStateProviderPtr = std::shared_ptr<SystemStateProvider>;

} // namespace vani::capabilities::system
