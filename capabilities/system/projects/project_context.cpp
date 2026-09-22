#include "project_context.hpp"

namespace vani::capabilities::system {

ProjectContextManager::ProjectContextManager() {
    init_sample_projects();
}

void ProjectContextManager::init_sample_projects() {
    Project p1;
    p1.id = "proj_vani_m2";
    p1.name = "VANI Mark 2";
    p1.root_path = "C:/Users/youri/OneDrive/Desktop/vani mark 2";
    p1.repository = "local";
    p1.language = "C++20 / TypeScript";
    p1.framework = "VANI Core Runtime / React";
    p1.known_commands["build"] = "cmake --build build";
    p1.known_commands["test"] = "ctest --test-dir build";
    p1.known_commands["dev"] = "npm run dev";
    p1.known_commands["verify"] = "node scripts/verify_phase4.cjs";
    register_project(p1);
    set_active_project("proj_vani_m2");

    Project p2;
    p2.id = "proj_react_frontend";
    p2.name = "React Frontend";
    p2.root_path = "/workspace/react-app";
    p2.repository = "github.com/org/react-app";
    p2.language = "TypeScript";
    p2.framework = "React / Vite";
    p2.known_commands["dev"] = "npm run dev";
    p2.known_commands["build"] = "npm run build";
    p2.known_commands["test"] = "npm test";
    register_project(p2);
}

void ProjectContextManager::register_project(Project project) {
    std::lock_guard<std::mutex> lock(mutex_);
    projects_[project.id] = std::move(project);
}

void ProjectContextManager::unregister_project(const std::string& project_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    projects_.erase(project_id);
    if (active_project_id_ == project_id) {
        active_project_id_.clear();
    }
}

void ProjectContextManager::set_active_project(const std::string& project_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (projects_.find(project_id) != projects_.end()) {
        active_project_id_ = project_id;
    }
}

std::optional<Project> ProjectContextManager::get_project(const std::string& project_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = projects_.find(project_id);
    if (it != projects_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<Project> ProjectContextManager::get_active_project() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_project_id_.empty()) {
        auto it = projects_.find(active_project_id_);
        if (it != projects_.end()) {
            return it->second;
        }
    }
    return std::nullopt;
}

std::vector<Project> ProjectContextManager::list_projects() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Project> result;
    result.reserve(projects_.size());
    for (const auto& [id, proj] : projects_) {
        result.push_back(proj);
    }
    return result;
}

} // namespace vani::capabilities::system
