#pragma once

#include <string>
#include <string_view>

namespace vani::capabilities::system {

class WorkingDirectoryResolver {
public:
    WorkingDirectoryResolver() = default;
    ~WorkingDirectoryResolver() = default;

    [[nodiscard]] static std::string resolve_cwd(
        std::string_view explicit_cwd,
        std::string_view task_cwd,
        std::string_view project_cwd,
        std::string_view session_cwd,
        std::string_view default_cwd = "/workspace"
    ) {
        if (!explicit_cwd.empty()) return std::string(explicit_cwd);
        if (!task_cwd.empty()) return std::string(task_cwd);
        if (!project_cwd.empty()) return std::string(project_cwd);
        if (!session_cwd.empty()) return std::string(session_cwd);
        return std::string(default_cwd);
    }
};

} // namespace vani::capabilities::system
