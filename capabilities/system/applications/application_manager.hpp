#pragma once

#include "application_registry.hpp"
#include "../../../adapters/system/system_adapter.hpp"
#include "../../../contracts/common/result.hpp"
#include <memory>
#include <string>
#include <vector>

namespace vani::capabilities::system {

class ApplicationManager {
public:
    ApplicationManager(
        adapters::system::SystemAdapterPtr adapter,
        ApplicationRegistryPtr registry
    );
    ~ApplicationManager() = default;

    contracts::Result<contracts::ApplicationMetadata> open_application(
        const std::string& name_or_alias,
        const std::vector<std::string>& args = {}
    );

    contracts::Result<void> close_application(
        const std::string& name_or_alias,
        bool force = false
    );

    contracts::Result<void> focus_application(const std::string& name_or_alias);
    contracts::Result<contracts::ApplicationMetadata> restart_application(const std::string& name_or_alias);

    contracts::Result<std::vector<contracts::ApplicationMetadata>> list_applications(bool running_only = false);
    [[nodiscard]] ApplicationRegistryPtr registry() const noexcept { return registry_; }

private:
    adapters::system::SystemAdapterPtr adapter_;
    ApplicationRegistryPtr registry_;
};

using ApplicationManagerPtr = std::shared_ptr<ApplicationManager>;

} // namespace vani::capabilities::system
