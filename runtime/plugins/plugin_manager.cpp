#include "plugin_manager.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace vani::runtime {

namespace fs = std::filesystem;

PluginManager::PluginManager(
    CapabilityRegistryPtr capability_registry,
    PermissionServicePtr permission_service,
    const std::string& plugins_dir
) : capability_registry_(std::move(capability_registry)),
    permission_service_(std::move(permission_service)),
    plugins_dir_(plugins_dir) {
}

contracts::Result<void> PluginManager::register_tool(contracts::ToolPtr tool) {
    if (!tool) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::ValidationError, "Cannot register null tool"
        );
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto m = tool->manifest();

    CapabilityDescriptor cap_desc;
    cap_desc.id = m.id;
    cap_desc.version = m.version;
    cap_desc.type = CapabilityType::Tool;
    cap_desc.provider_id = "plugin.tool";
    cap_desc.description = m.description;
    cap_desc.risk_level = m.risk_level;
    cap_desc.required_permissions = m.required_permissions;
    cap_desc.supports_cancellation = true;
    cap_desc.availability = CapabilityAvailability::Available;

    if (capability_registry_) {
        auto reg_res = capability_registry_->register_capability(cap_desc);
        if (reg_res.is_err()) {
            return reg_res;
        }
    }

    registered_tools_.push_back(tool);
    return contracts::Result<void>::success();
}

contracts::Result<void> PluginManager::register_agent(contracts::AgentPtr agent) {
    if (!agent) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::ValidationError, "Cannot register null agent"
        );
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto m = agent->manifest();

    CapabilityDescriptor cap_desc;
    cap_desc.id = m.id;
    cap_desc.version = m.version;
    cap_desc.type = CapabilityType::Agent;
    cap_desc.provider_id = "plugin.agent";
    cap_desc.description = m.description;
    cap_desc.required_permissions = m.required_permissions;
    cap_desc.availability = CapabilityAvailability::Available;

    if (capability_registry_) {
        auto reg_res = capability_registry_->register_capability(cap_desc);
        if (reg_res.is_err()) {
            return reg_res;
        }
    }

    registered_agents_.push_back(agent);
    return contracts::Result<void>::success();
}

contracts::Result<void> PluginManager::load_plugin(contracts::PluginPtr plugin) {
    if (!plugin) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::ValidationError, "Cannot load null plugin"
        );
    }

    auto manifest = plugin->manifest();
    if (manifest.id.empty()) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::ValidationError, "Plugin manifest id cannot be empty"
        );
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (plugins_.find(manifest.id) != plugins_.end()) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::AlreadyExists, "Plugin already loaded: " + manifest.id
        );
    }

    // Grant requested permissions scoped to this plugin
    if (permission_service_) {
        std::string actor = "plugin." + manifest.id;
        for (const auto& perm : manifest.requested_permissions) {
            permission_service_->grant_permission(actor, perm);
        }
    }

    auto load_res = plugin->on_load(*this);
    if (load_res.is_err()) {
        return load_res;
    }

    plugins_[manifest.id] = plugin;
    return contracts::Result<void>::success();
}

contracts::Result<void> PluginManager::unload_plugin(const std::string& plugin_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = plugins_.find(plugin_id);
    if (it == plugins_.end()) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::NotFound, "Plugin not found: " + plugin_id
        );
    }

    it->second->on_unload();
    plugins_.erase(it);
    return contracts::Result<void>::success();
}

contracts::Result<size_t> PluginManager::scan_and_load_all() {
    try {
        if (!fs::exists(plugins_dir_)) {
            fs::create_directories(plugins_dir_);
            return contracts::Result<size_t>::success(0);
        }

        size_t count = 0;
        for (const auto& entry : fs::directory_iterator(plugins_dir_)) {
            if (entry.is_directory()) {
                fs::path manifest_path = entry.path() / "plugin.json";
                if (fs::exists(manifest_path)) {
                    // Discovered a valid plugin directory with manifest
                    count++;
                }
            }
        }

        return contracts::Result<size_t>::success(count);
    } catch (const std::exception& e) {
        return contracts::Result<size_t>::failure(
            contracts::ErrorCode::InternalError, std::string("Plugin scan failed: ") + e.what()
        );
    }
}

std::vector<contracts::PluginManifest> PluginManager::list_loaded_plugins() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<contracts::PluginManifest> list;
    list.reserve(plugins_.size());
    for (const auto& [id, plugin] : plugins_) {
        list.push_back(plugin->manifest());
    }
    return list;
}

size_t PluginManager::loaded_count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return plugins_.size();
}

bool PluginManager::is_loaded(const std::string& plugin_id) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return plugins_.find(plugin_id) != plugins_.end();
}

} // namespace vani::runtime
