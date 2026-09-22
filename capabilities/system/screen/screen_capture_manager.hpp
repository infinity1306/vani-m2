#pragma once

#include "../../../adapters/system/system_adapter.hpp"
#include "../../../contracts/common/result.hpp"
#include <memory>
#include <string>

namespace vani::capabilities::system {

class ScreenCaptureManager {
public:
    explicit ScreenCaptureManager(adapters::system::SystemAdapterPtr adapter);
    ~ScreenCaptureManager() = default;

    contracts::Result<contracts::ScreenCaptureMetadata> capture_screen(
        uint32_t monitor_id = 0,
        uint32_t window_id = 0
    );

    [[nodiscard]] bool is_local_storage_only() const noexcept { return local_storage_only_; }
    void set_local_storage_only(bool local_only) noexcept { local_storage_only_ = local_only; }

private:
    adapters::system::SystemAdapterPtr adapter_;
    bool local_storage_only_{true};
};

using ScreenCaptureManagerPtr = std::shared_ptr<ScreenCaptureManager>;

} // namespace vani::capabilities::system
