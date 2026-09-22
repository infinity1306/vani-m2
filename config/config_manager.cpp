#include "config_manager.hpp"

namespace vani::config {

ConfigManager::ConfigManager(RuntimeProfile profile)
    : profile_(std::move(profile)) {}

const RuntimeProfile& ConfigManager::profile() const noexcept {
    return profile_;
}

void ConfigManager::set_override(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    overrides_[key] = value;
}

std::optional<std::string> ConfigManager::get_override(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = overrides_.find(key);
    if (it != overrides_.end()) {
        return it->second;
    }
    return std::nullopt;
}

contracts::Result<void> ConfigManager::validate() const {
    if (profile_.default_stt_engine.empty()) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::ValidationError,
            "Config error: default_stt_engine cannot be empty",
            "vani.config"
        );
    }
    if (profile_.default_model_provider.empty()) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::ValidationError,
            "Config error: default_model_provider cannot be empty",
            "vani.config"
        );
    }
    return contracts::Result<void>::ok();
}

} // namespace vani::config
