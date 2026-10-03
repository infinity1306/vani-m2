#pragma once

#include "../../contracts/plugins/plugin.hpp"
#include "../../contracts/common/result.hpp"
#include "../capability_registry/capability_registry.hpp"
#include "../permissions/permission_service.hpp"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace vani::runtime {

class PluginManager : public contracts::PluginContext {
public:
    PluginManager(
        CapabilityRegistryPtr capability_registry,
        PermissionServicePtr permission_service,
        const std::string& plugins_dir = "plugins"
    );
    ~PluginManager() override = default;

    // contracts::PluginContext overrides
    contracts::Result<void> register_tool(contracts::ToolPtr tool) override;
    contracts::Result<void> register_agent(contracts::AgentPtr agent) override;

    // Lifecycle
    contracts::Result<void> load_plugin(contracts::PluginPtr plugin);
    contracts::Result<void> unload_plugin(const std::string& plugin_id);
    contracts::Result<size_t> scan_and_load_all();

    [[nodiscard]] std::vector<contracts::PluginManifest> list_loaded_plugins() const;
    [[nodiscard]] size_t loaded_count() const noexcept;
    [[nodiscard]] bool is_loaded(const std::string& plugin_id) const noexcept;

private:
    mutable std::mutex mutex_;
    CapabilityRegistryPtr capability_registry_;
    PermissionServicePtr permission_service_;
    std::string plugins_dir_;
    std::unordered_map<std::string, contracts::PluginPtr> plugins_;
    std::vector<contracts::ToolPtr> registered_tools_;
    std::vector<contracts::AgentPtr> registered_agents_;
};

using PluginManagerPtr = std::shared_ptr<PluginManager>;

} // namespace vani::runtime
