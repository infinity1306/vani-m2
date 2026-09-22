#include "media_manager.hpp"

namespace vani::capabilities::system {

MediaManager::MediaManager(adapters::system::SystemAdapterPtr adapter)
    : adapter_(std::move(adapter)) {}

contracts::Result<contracts::MediaStatus> MediaManager::get_status() {
    if (!adapter_) {
        return contracts::Result<contracts::MediaStatus>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->get_media_status();
}

contracts::Result<void> MediaManager::play() {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->media_play();
}

contracts::Result<void> MediaManager::pause() {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->media_pause();
}

contracts::Result<void> MediaManager::stop() {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->media_stop();
}

contracts::Result<void> MediaManager::next() {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->media_next();
}

contracts::Result<void> MediaManager::previous() {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->media_previous();
}

contracts::Result<void> MediaManager::set_volume(uint32_t volume_percent) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->set_volume(volume_percent);
}

} // namespace vani::capabilities::system
