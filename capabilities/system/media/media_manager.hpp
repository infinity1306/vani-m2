#pragma once

#include "../../../adapters/system/system_adapter.hpp"
#include "../../../contracts/common/result.hpp"
#include <memory>

namespace vani::capabilities::system {

class MediaManager {
public:
    explicit MediaManager(adapters::system::SystemAdapterPtr adapter);
    ~MediaManager() = default;

    contracts::Result<contracts::MediaStatus> get_status();
    contracts::Result<void> play();
    contracts::Result<void> pause();
    contracts::Result<void> stop();
    contracts::Result<void> next();
    contracts::Result<void> previous();
    contracts::Result<void> set_volume(uint32_t volume_percent);

private:
    adapters::system::SystemAdapterPtr adapter_;
};

using MediaManagerPtr = std::shared_ptr<MediaManager>;

} // namespace vani::capabilities::system
