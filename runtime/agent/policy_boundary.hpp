#pragma once

#include "../../contracts/agents/agent_execution_contracts.hpp"
#include "../policy/policy_engine.hpp"
#include <memory>
#include <functional>

namespace vani::runtime::agent {

struct PolicyEvaluationOutcome {
    bool allowed{false};
    bool requires_confirmation{false};
    std::string decision_str;
    std::string reason;
    contracts::AgentConfirmationRequest confirmation_payload;
};

using ConfirmationHandler = std::function<bool(const contracts::AgentConfirmationRequest&)>;

class PolicyBoundary {
public:
    explicit PolicyBoundary(runtime::PolicyEnginePtr policy_engine);
    ~PolicyBoundary() = default;

    [[nodiscard]] PolicyEvaluationOutcome evaluate_step(
        const contracts::AgentStep& step,
        const contracts::AgentRequest& request,
        const std::string& plan_id
    ) const;

    void set_confirmation_handler(ConfirmationHandler handler) {
        confirmation_handler_ = std::move(handler);
    }

    [[nodiscard]] bool prompt_confirmation(const contracts::AgentConfirmationRequest& req) const {
        if (confirmation_handler_) {
            return confirmation_handler_(req);
        }
        return false; // Default safe: denied if no confirmation handler attached
    }

private:
    runtime::PolicyEnginePtr policy_engine_;
    ConfirmationHandler confirmation_handler_{nullptr};
};

using PolicyBoundaryPtr = std::shared_ptr<PolicyBoundary>;

} // namespace vani::runtime::agent
