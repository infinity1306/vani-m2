#pragma once

#include "../common/version.hpp"
#include "../common/result.hpp"
#include "../tools/tool.hpp"
#include "../agents/agent.hpp"
#include <string>
#include <vector>
#include <memory>

namespace vani::contracts {

struct PluginManifest {
    std::string id;
    SemanticVersion version{1, 0, 0};
    std::string name;
    std::string author;
    std::string description;
    std::vector<std::string> requested_permissions;
    bool sandboxed{true};
};

class PluginContext {
public:
    virtual ~PluginContext() = default;

    virtual Result<void> register_tool(ToolPtr tool) = 0;
    virtual Result<void> register_agent(AgentPtr agent) = 0;
};

class Plugin {
public:
    virtual ~Plugin() = default;

    [[nodiscard]] virtual PluginManifest manifest() const = 0;

    virtual Result<void> on_load(PluginContext& context) = 0;

    virtual Result<void> on_unload() = 0;
};

using PluginPtr = std::shared_ptr<Plugin>;

} // namespace vani::contracts
