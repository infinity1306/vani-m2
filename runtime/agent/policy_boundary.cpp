#include "policy_boundary.hpp"

namespace vani::runtime::agent {

PolicyBoundary::PolicyBoundary(runtime::PolicyEnginePtr policy_engine)
    : policy_engine_(std::move(policy_engine)) {}

PolicyEvaluationOutcome PolicyBoundary::evaluate_step(
    const contracts::AgentStep& step,
    const contracts::AgentRequest& request,
    const std::string& plan_id
) const {
    PolicyEvaluationOutcome outcome;

    if (!policy_engine_) {
        // Safe default: if no policy engine is attached, deny execution
        outcome.allowed = false;
        outcome.requires_confirmation = false;
        outcome.decision_str = "POLICY_DENIED";
        outcome.reason = "No PolicyEngine attached to agent boundary";
        return outcome;
    }

    // Strictly construct PolicyContext with agent actor
    PolicyContext ctx;
    ctx.actor_id = "agent.planner";
    ctx.capability_id = step.capability_id;
    ctx.risk_level = step.risk_level;
    ctx.user_id = request.permission_context.empty() ? "user.agentic" : request.permission_context;
    ctx.session_id = request.originating_session;
    ctx.task_id = request.request_id;
    ctx.is_local_execution = true;
    ctx.is_user_present = true;

    // Evaluate policy decision
    PolicyDecision decision = policy_engine_->evaluate(ctx);

    outcome.decision_str = std::string(to_string(decision));

    switch (decision) {
        case PolicyDecision::Allow:
        case PolicyDecision::AllowWithAudit:
            outcome.allowed = true;
            outcome.requires_confirmation = false;
            outcome.reason = "Policy allowed step execution";
            break;

        case PolicyDecision::RequireConfirmation:
            outcome.allowed = false;
            outcome.requires_confirmation = true;
            outcome.reason = "Policy requires explicit user confirmation";
            outcome.confirmation_payload.request_id = request.request_id;
            outcome.confirmation_payload.plan_id = plan_id;
            outcome.confirmation_payload.step_id = step.step_id;
            outcome.confirmation_payload.intended_action = step.capability_id;
            outcome.confirmation_payload.risk_level = step.risk_level;
            outcome.confirmation_payload.expected_effect = step.expected_postcondition;
            
            // Extract target from arguments if present
            if (step.arguments.find("app_name") != step.arguments.end()) {
                outcome.confirmation_payload.target = step.arguments.at("app_name");
            } else if (step.arguments.find("url") != step.arguments.end()) {
                outcome.confirmation_payload.target = step.arguments.at("url");
            } else if (step.arguments.find("path") != step.arguments.end()) {
                outcome.confirmation_payload.target = step.arguments.at("path");
            } else {
                outcome.confirmation_payload.target = step.tool_id;
            }
            outcome.confirmation_payload.reason = "Agent step requires user approval: " + step.capability_id;
            break;

        case PolicyDecision::Deny:
        default:
            outcome.allowed = false;
            outcome.requires_confirmation = false;
            outcome.reason = "Policy explicitly denied execution for capability: " + step.capability_id;
            break;
    }

    return outcome;
}

} // namespace vani::runtime::agent
