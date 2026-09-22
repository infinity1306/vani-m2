#pragma once

#include "../common/version.hpp"
#include "../common/result.hpp"
#include "../common/cancellation_token.hpp"
#include "../tasks/task.hpp"
#include <string>
#include <vector>
#include <memory>

namespace vani::contracts {

struct AgentManifest {
    AgentId id;
    SemanticVersion version{1, 0, 0};
    std::string name;
    std::string role; // e.g. "Coding", "Research", "Automation"
    std::string description;
    std::vector<std::string> provided_capabilities;
    std::vector<std::string> required_tools;
    std::vector<std::string> required_permissions;
    bool supports_cancellation{true};
};

struct AgentRequest {
    TaskId task_id;
    SessionId session_id;
    std::string instruction;
    std::vector<std::string> allowed_tools;
    std::string model_id_preference;
    CancellationToken cancellation_token{CancellationToken::none()};
};

struct AgentResult {
    TaskId task_id;
    bool success{false};
    std::string output_summary;
    std::vector<TaskArtifact> produced_artifacts;
    std::vector<std::string> tools_used;
    uint32_t total_steps{0};
};

class Agent {
public:
    virtual ~Agent() = default;

    [[nodiscard]] virtual AgentManifest manifest() const = 0;

    virtual Result<AgentResult> run(const AgentRequest& request) = 0;

    virtual Result<void> cancel(const TaskId& task_id) = 0;
};

using AgentPtr = std::shared_ptr<Agent>;

} // namespace vani::contracts
