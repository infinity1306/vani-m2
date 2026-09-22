#pragma once

#include "../../contracts/agents/agent.hpp"

namespace vani::adapters {

class MockAgentAdapter : public contracts::Agent {
public:
    explicit MockAgentAdapter(std::string agent_id = "agent.odysseus", std::string role = "Coding")
        : id_(std::move(agent_id)), role_(std::move(role)) {}

    [[nodiscard]] contracts::AgentManifest manifest() const override {
        return contracts::AgentManifest{
            .id = id_,
            .version = {1, 0, 0},
            .name = "Mock " + role_ + " Agent",
            .role = role_,
            .description = "Autonomous task solver adhering strictly to VANI capabilities.",
            .provided_capabilities = {"coding.refactor", "coding.test"},
            .required_tools = {"terminal", "filesystem", "git"},
            .required_permissions = {"terminal.execute", "filesystem.write"},
            .supports_cancellation = true
        };
    }

    contracts::Result<contracts::AgentResult> run(
        const contracts::AgentRequest& request
    ) override {
        if (request.cancellation_token.is_cancelled()) {
            return contracts::Result<contracts::AgentResult>::err(
                contracts::ErrorCode::Cancelled,
                "Agent execution cancelled by user.",
                "adapter." + id_
            );
        }

        return contracts::Result<contracts::AgentResult>(contracts::AgentResult{
            .task_id = request.task_id,
            .success = true,
            .output_summary = "Agent successfully synthesized code changes and passed tests.",
            .produced_artifacts = {
                contracts::TaskArtifact{
                    .id = "art_1",
                    .name = "patch.diff",
                    .type = "diff",
                    .path_or_content = "+ // VANI Mutex Patch",
                    .size_bytes = 48
                }
            },
            .tools_used = {"filesystem", "terminal"},
            .total_steps = 3
        });
    }

    contracts::Result<void> cancel(const contracts::TaskId& /*task_id*/) override {
        return contracts::Result<void>::ok();
    }

private:
    std::string id_;
    std::string role_;
};

} // namespace vani::adapters
