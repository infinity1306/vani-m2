#pragma once

#include "task_state.hpp"
#include "../common/version.hpp"
#include "../common/result.hpp"
#include <string>
#include <vector>
#include <chrono>
#include <optional>

namespace vani::contracts {

struct TaskArtifact {
    std::string id;
    std::string name;
    std::string type;
    std::string path_or_content;
    uint64_t size_bytes{0};
};

struct TaskSpecification {
    TaskId task_id;
    SessionId session_id;
    std::optional<TaskId> parent_task_id;
    std::string title;
    std::string description;
    std::string category{"general"};
    TaskPriority priority{TaskPriority::Normal};
    std::vector<std::string> requested_capabilities;
    std::vector<std::string> required_permissions;
    std::string assigned_agent_id;
    uint32_t timeout_seconds{300};
};

struct TaskResult {
    TaskId task_id;
    bool success{true};
    std::string summary;
    std::string output_summary;
    std::string structured_data_json;
    std::vector<TaskArtifact> artifacts;
    std::optional<Error> error;
};

struct Task {
    TaskSpecification spec;
    TaskState state{TaskState::Created};
    uint8_t progress_percent{0};
    uint64_t created_at_ms{0};
    uint64_t updated_at_ms{0};
    std::optional<uint64_t> completed_at_ms{std::nullopt};
    std::optional<TaskResult> result{std::nullopt};
    std::optional<Error> error{std::nullopt};
    std::optional<std::string> checkpoint{std::nullopt};
    std::vector<TaskArtifact> artifacts;
    std::vector<std::string> execution_logs;
    std::string checkpoint_id;
    std::string result_summary;

    [[nodiscard]] const TaskId& id() const noexcept { return spec.task_id; }
    [[nodiscard]] bool is_terminal() const noexcept {
        return state == TaskState::Completed ||
               state == TaskState::Failed ||
               state == TaskState::Cancelled;
    }
};

} // namespace vani::contracts
