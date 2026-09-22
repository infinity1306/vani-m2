#include "application_manager.hpp"

namespace vani::capabilities::system {

ApplicationManager::ApplicationManager(
    adapters::system::SystemAdapterPtr adapter,
    ApplicationRegistryPtr registry
) : adapter_(std::move(adapter)), registry_(std::move(registry)) {}

contracts::Result<contracts::ApplicationMetadata> ApplicationManager::open_application(
    const std::string& name_or_alias,
    const std::vector<std::string>& args
) {
    if (!adapter_) {
        return contracts::Result<contracts::ApplicationMetadata>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }

    std::string target = name_or_alias;
    if (registry_) {
        if (auto resolved = registry_->resolve(name_or_alias)) {
            target = resolved->application_id;
        }
    }

    return adapter_->launch_application(target, args);
}

contracts::Result<void> ApplicationManager::close_application(
    const std::string& name_or_alias,
    bool force
) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }

    std::string target = name_or_alias;
    if (registry_) {
        if (auto resolved = registry_->resolve(name_or_alias)) {
            target = resolved->application_id;
        }
    }

    return adapter_->terminate_application(target, force);
}

contracts::Result<void> ApplicationManager::focus_application(const std::string& name_or_alias) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }

    std::string target = name_or_alias;
    if (registry_) {
        if (auto resolved = registry_->resolve(name_or_alias)) {
            target = resolved->application_id;
        }
    }

    return adapter_->focus_application(target);
}

contracts::Result<contracts::ApplicationMetadata> ApplicationManager::restart_application(const std::string& name_or_alias) {
    auto close_res = close_application(name_or_alias, false);
    if (!close_res.is_success()) {
        return contracts::Result<contracts::ApplicationMetadata>::failure(
            close_res.error()
        );
    }
    return open_application(name_or_alias);
}

contracts::Result<std::vector<contracts::ApplicationMetadata>> ApplicationManager::list_applications(bool running_only) {
    if (!adapter_) {
        return contracts::Result<std::vector<contracts::ApplicationMetadata>>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }

    if (running_only) {
        return adapter_->list_running_applications();
    }
    return adapter_->list_installed_applications();
}

} // namespace vani::capabilities::system
