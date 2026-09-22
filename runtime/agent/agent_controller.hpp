#pragma once

#include "../../contracts/agents/agent_execution_contracts.hpp"
#include "../../capabilities/system/gateway/tool_gateway.hpp"
#include "plan_validator.hpp"
#include "policy_boundary.hpp"
#include "postcondition_verifier.hpp"
#include "execution_scheduler.hpp"
#include "replanner.hpp"
#include "agent_memory.hpp"
#include "agent_model_provider.hpp"
#include "complexity_classifier.hpp"
#include "agent_audit_journal.hpp"
#include <memory>
#include <string>

namespace vani::runtime::agent {

class AgentController {
public:
    AgentController(
        capabilities::system::ToolGatewayPtr gateway,
        runtime::PolicyEnginePtr policy_engine,
        AgentModelProviderPtr model_provider = nullptr
    );
    ~AgentController() = default;

    // Execute an agentic goal
    contracts::AgentExecutionResult execute_goal(const contracts::AgentRequest& request);

    // Complexity check
    [[nodiscard]] ClassificationDecision classify_request(
        const std::string& raw_text,
        const std::string& normalized_text,
        const std::string& intent
    ) const;

    // Subsystem accessors
    [[nodiscard]] PlanValidator& validator() noexcept { return validator_; }
    [[nodiscard]] PolicyBoundaryPtr policy_boundary() const noexcept { return policy_boundary_; }
    [[nodiscard]] AgentAuditJournalPtr audit_journal() const noexcept { return audit_journal_; }
    [[nodiscard]] ShortTermTaskMemoryPtr memory() const noexcept { return memory_; }
    [[nodiscard]] AgentModelProviderPtr model_provider() const noexcept { return model_provider_; }

    void set_model_provider(AgentModelProviderPtr provider) { model_provider_ = std::move(provider); }
    void set_simulated_free_ram_mb(uint64_t mb) { simulated_free_ram_mb_ = mb; }
    void set_require_real_model(bool required) noexcept { require_real_model_ = required; }
    [[nodiscard]] bool require_real_model() const noexcept { return require_real_model_; }

    [[nodiscard]] contracts::AgentPlan last_initial_plan() const { return last_initial_plan_; }
    [[nodiscard]] contracts::AgentPlan last_replanned_plan() const { return last_replanned_plan_; }

private:
    capabilities::system::ToolGatewayPtr gateway_;
    runtime::PolicyEnginePtr policy_engine_;
    AgentModelProviderPtr model_provider_;

    PlanValidator validator_;
    PolicyBoundaryPtr policy_boundary_;
    PostconditionVerifierPtr verifier_;
    ExecutionSchedulerPtr scheduler_;
    Replanner replanner_;
    ShortTermTaskMemoryPtr memory_;
    ComplexityClassifier classifier_;
    AgentAuditJournalPtr audit_journal_;

    uint64_t simulated_free_ram_mb_{4096};
    bool require_real_model_{false};
    contracts::AgentPlan last_initial_plan_;
    contracts::AgentPlan last_replanned_plan_;
};

using AgentControllerPtr = std::shared_ptr<AgentController>;

} // namespace vani::runtime::agent
