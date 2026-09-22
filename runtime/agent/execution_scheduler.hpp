#pragma once

#include "../../contracts/agents/agent_execution_contracts.hpp"
#include "../../capabilities/system/gateway/tool_gateway.hpp"
#include "policy_boundary.hpp"
#include "postcondition_verifier.hpp"
#include <memory>
#include <functional>

namespace vani::runtime::agent {

using StepStatusCallback = std::function<void(const contracts::AgentStep&, const contracts::AgentObservation&)>;

class ExecutionScheduler {
public:
    ExecutionScheduler(
        capabilities::system::ToolGatewayPtr gateway,
        PolicyBoundaryPtr policy_boundary,
        PostconditionVerifierPtr verifier
    );
    ~ExecutionScheduler() = default;

    contracts::AgentExecutionResult execute_plan(
        const contracts::AgentPlan& plan,
        const contracts::AgentRequest& request,
        StepStatusCallback callback = nullptr
    );

    void set_step_delay_ms(uint32_t ms) { step_delay_ms_ = ms; }
    void set_simulated_transient_failures(int count) { transient_failures_remaining_ = count; }

private:
    capabilities::system::ToolGatewayPtr gateway_;
    PolicyBoundaryPtr policy_boundary_;
    PostconditionVerifierPtr verifier_;
    uint32_t step_delay_ms_{0};
    int transient_failures_remaining_{0};
};

using ExecutionSchedulerPtr = std::shared_ptr<ExecutionScheduler>;

} // namespace vani::runtime::agent
