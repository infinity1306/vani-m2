#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <optional>

namespace vani::capabilities::system {

struct Project {
    std::string id;
    std::string name;
    std::string root_path;
    std::string repository;
    std::string language;
    std::string framework;
    std::unordered_map<std::string, std::string> known_commands;
};

class ProjectContextManager {
public:
    ProjectContextManager();
    ~ProjectContextManager() = default;

    void register_project(Project project);
    void unregister_project(const std::string& project_id);
    void set_active_project(const std::string& project_id);

    [[nodiscard]] std::optional<Project> get_project(const std::string& project_id) const;
    [[nodiscard]] std::optional<Project> get_active_project() const;
    [[nodiscard]] std::vector<Project> list_projects() const;

private:
    void init_sample_projects();

    mutable std::mutex mutex_;
    std::unordered_map<std::string, Project> projects_;
    std::string active_project_id_;
};

using ProjectContextManagerPtr = std::shared_ptr<ProjectContextManager>;

} // namespace vani::capabilities::system
