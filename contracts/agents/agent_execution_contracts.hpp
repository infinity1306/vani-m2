#pragma once

#include "../common/result.hpp"
#include "../common/cancellation_token.hpp"
#include "../tools/risk_level.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <memory>
#include <cstdint>

namespace vani::contracts {

// ============================================================================
// 1. Complexity Route
// ============================================================================
enum class ComplexityRoute : uint8_t {
    FastPath,
    AgentPath
};

[[nodiscard]] constexpr std::string_view to_string(ComplexityRoute route) noexcept {
    switch (route) {
        case ComplexityRoute::FastPath: return "FAST_PATH";
        case ComplexityRoute::AgentPath: return "AGENT_PATH";
        default: return "FAST_PATH";
    }
}

// ============================================================================
// 2. Plan State Machine
// ============================================================================
enum class PlanState : uint8_t {
    Created,
    Planning,
    Validating,
    WaitingForPolicy,
    WaitingForConfirmation,
    Ready,
    Executing,
    Observing,
    Replanning,
    Completed,
    // Failure / Terminal States
    Failed,
    Cancelled,
    Timeout,
    BlockedPolicy,
    BlockedResource,
    BlockedDependency
};

[[nodiscard]] constexpr std::string_view to_string(PlanState state) noexcept {
    switch (state) {
        case PlanState::Created: return "CREATED";
        case PlanState::Planning: return "PLANNING";
        case PlanState::Validating: return "VALIDATING";
        case PlanState::WaitingForPolicy: return "WAITING_FOR_POLICY";
        case PlanState::WaitingForConfirmation: return "WAITING_FOR_CONFIRMATION";
        case PlanState::Ready: return "READY";
        case PlanState::Executing: return "EXECUTING";
        case PlanState::Observing: return "OBSERVING";
        case PlanState::Replanning: return "REPLANNING";
        case PlanState::Completed: return "COMPLETED";
        case PlanState::Failed: return "FAILED";
        case PlanState::Cancelled: return "CANCELLED";
        case PlanState::Timeout: return "TIMEOUT";
        case PlanState::BlockedPolicy: return "BLOCKED_POLICY";
        case PlanState::BlockedResource: return "BLOCKED_RESOURCE";
        case PlanState::BlockedDependency: return "BLOCKED_DEPENDENCY";
        default: return "UNKNOWN";
    }
}

[[nodiscard]] constexpr bool is_terminal_state(PlanState state) noexcept {
    return state == PlanState::Completed ||
           state == PlanState::Failed ||
           state == PlanState::Cancelled ||
           state == PlanState::Timeout ||
           state == PlanState::BlockedPolicy ||
           state == PlanState::BlockedResource ||
           state == PlanState::BlockedDependency;
}

// ============================================================================
// 3. Resource Budget
// ============================================================================
struct AgentResourceBudget {
    uint32_t max_steps{20};
    uint32_t max_tool_calls{30};
    uint32_t max_replans{3};
    uint32_t max_retries_per_step{2};
    uint32_t max_context_tokens{4096};
    uint64_t global_timeout_ms{300000};     // 5 minutes
    uint64_t default_step_timeout_ms{30000}; // 30 seconds
};

// ============================================================================
// 4. Agent Request
// ============================================================================
struct AgentRequest {
    std::string request_id;
    std::string goal;
    std::string context;
    std::string originating_session{"session_default"};
    uint32_t priority{1};
    uint64_t deadline_ms{0};
    CancellationToken cancellation_token{CancellationToken::none()};
    AgentResourceBudget resource_budget{};
    std::string permission_context{"user.agentic"};
};

// ============================================================================
// 5. Agent Step & Plan
// ============================================================================
struct AgentStep {
    std::string step_id;
    std::string capability_id;
    std::string tool_id;
    std::unordered_map<std::string, std::string> arguments;
    std::vector<std::string> dependencies;
    std::string expected_postcondition;
    uint64_t timeout_ms{30000};
    uint32_t retry_policy_max_attempts{2};
    RiskLevel risk_level{RiskLevel::Low};
};

struct AgentPlan {
    std::string plan_id;
    std::string request_id;
    std::vector<AgentStep> steps;
    std::vector<std::pair<std::string, std::string>> dependencies; // from -> to
    std::vector<std::string> expected_outcomes;
    RiskLevel risk_level{RiskLevel::Low};
    double estimated_cost{0.0};
    uint64_t created_at_ms{0};
    std::string planner_provider{"deterministic_rule_planner"};
};

// ============================================================================
// 6. Agent Observation
// ============================================================================
struct AgentObservation {
    std::string step_id;
    bool success{false};
    std::string result_data;
    bool postcondition_verified{false};
    std::string evidence;
    std::string error;
    uint64_t timestamp_ms{0};
    uint32_t attempts_taken{1};
};

// ============================================================================
// 7. Confirmation Payload
// ============================================================================
struct AgentConfirmationRequest {
    std::string request_id;
    std::string plan_id;
    std::string step_id;
    std::string intended_action;
    std::string target;
    std::string reason;
    RiskLevel risk_level{RiskLevel::High};
    std::string expected_effect;
};

// ============================================================================
// 8. Final Agent Execution Outcome
// ============================================================================
struct AgentExecutionResult {
    std::string request_id;
    std::string plan_id;
    PlanState final_state{PlanState::Failed};
    std::string final_response;
    std::vector<AgentObservation> observations;
    uint32_t total_steps_executed{0};
    uint32_t total_tool_calls{0};
    uint32_t total_replans{0};
    uint32_t total_retries{0};
    uint64_t duration_ms{0};
    std::string error_message;
    bool verified{false};
};

} // namespace vani::contracts
