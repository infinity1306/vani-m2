#include "../../contracts/agents/agent_execution_contracts.hpp"
#include "../../runtime/agent/agent_telemetry.hpp"
#include <iostream>
#include <cassert>

using namespace vani::contracts;
using namespace vani::runtime::agent;

void test_contracts_defaults() {
    AgentResourceBudget budget;
    assert(budget.max_steps == 20);
    assert(budget.max_tool_calls == 30);
    assert(budget.max_replans == 3);
    assert(budget.max_retries_per_step == 2);
    assert(budget.max_context_tokens == 4096);
    assert(budget.global_timeout_ms == 300000);
    assert(budget.default_step_timeout_ms == 30000);

    AgentRequest req;
    req.request_id = "req_101";
    req.goal = "Open Chrome and search for VANI";
    req.originating_session = "sess_main";
    req.priority = 2;
    assert(req.request_id == "req_101");
    assert(req.goal == "Open Chrome and search for VANI");
    assert(!req.cancellation_token.is_cancelled());

    AgentStep s;
    s.step_id = "s1";
    s.capability_id = "application.launch";
    s.tool_id = "app_manager.launch";
    s.arguments["app_name"] = "chrome.exe";
    s.expected_postcondition = "process.running:chrome.exe";
    s.risk_level = RiskLevel::Low;
    assert(s.step_id == "s1");
    assert(s.arguments["app_name"] == "chrome.exe");

    AgentPlan plan;
    plan.plan_id = "p1";
    plan.request_id = "req_101";
    plan.steps.push_back(s);
    plan.planner_provider = "deterministic_rule_planner";
    assert(plan.steps.size() == 1);
    assert(plan.planner_provider == "deterministic_rule_planner");

    std::cout << "[PASS] test_contracts_defaults\n";
}

void test_plan_state_machine_enum() {
    assert(to_string(PlanState::Created) == "CREATED");
    assert(to_string(PlanState::Planning) == "PLANNING");
    assert(to_string(PlanState::Validating) == "VALIDATING");
    assert(to_string(PlanState::WaitingForPolicy) == "WAITING_FOR_POLICY");
    assert(to_string(PlanState::WaitingForConfirmation) == "WAITING_FOR_CONFIRMATION");
    assert(to_string(PlanState::Ready) == "READY");
    assert(to_string(PlanState::Executing) == "EXECUTING");
    assert(to_string(PlanState::Observing) == "OBSERVING");
    assert(to_string(PlanState::Replanning) == "REPLANNING");
    assert(to_string(PlanState::Completed) == "COMPLETED");
    assert(to_string(PlanState::Failed) == "FAILED");
    assert(to_string(PlanState::Cancelled) == "CANCELLED");
    assert(to_string(PlanState::Timeout) == "TIMEOUT");
    assert(to_string(PlanState::BlockedPolicy) == "BLOCKED_POLICY");
    assert(to_string(PlanState::BlockedResource) == "BLOCKED_RESOURCE");
    assert(to_string(PlanState::BlockedDependency) == "BLOCKED_DEPENDENCY");

    assert(is_terminal_state(PlanState::Completed));
    assert(is_terminal_state(PlanState::Failed));
    assert(is_terminal_state(PlanState::Cancelled));
    assert(is_terminal_state(PlanState::Timeout));
    assert(is_terminal_state(PlanState::BlockedPolicy));
    assert(is_terminal_state(PlanState::BlockedResource));
    assert(is_terminal_state(PlanState::BlockedDependency));
    assert(!is_terminal_state(PlanState::Executing));
    assert(!is_terminal_state(PlanState::Planning));

    std::cout << "[PASS] test_plan_state_machine_enum\n";
}

void test_agent_telemetry_phases() {
    auto& col = AgentTelemetryCollector::instance();
    col.clear();
    assert(col.total_records() == 0);

    col.record(AgentTelemetryPhase::A0_AgentRequest, "task_1", "", "", "User goal");
    col.record(AgentTelemetryPhase::A1_PlanningStart, "task_1", "plan_1");
    col.record(AgentTelemetryPhase::A2_PlanningComplete, "task_1", "plan_1", "", "2 steps");
    col.record(AgentTelemetryPhase::A3_ValidationComplete, "task_1", "plan_1");
    col.record(AgentTelemetryPhase::A4_PolicyCheck, "task_1", "plan_1", "step_1");
    col.record(AgentTelemetryPhase::A5_StepStart, "task_1", "plan_1", "step_1");
    col.record(AgentTelemetryPhase::A6_ToolRequest, "task_1", "plan_1", "step_1");
    col.record(AgentTelemetryPhase::A7_ToolComplete, "task_1", "plan_1", "step_1");
    col.record(AgentTelemetryPhase::A8_Verification, "task_1", "plan_1", "step_1");
    col.record(AgentTelemetryPhase::A9_Observation, "task_1", "plan_1", "step_1", "verified OK");
    col.record(AgentTelemetryPhase::A10_Replan, "task_1", "plan_1", "step_1");
    col.record(AgentTelemetryPhase::A11_TaskComplete, "task_1", "plan_1");
    col.record(AgentTelemetryPhase::A12_TaskFailed, "task_1", "plan_1");
    col.record(AgentTelemetryPhase::A13_Cancellation, "task_1", "plan_1");

    assert(col.total_records() == 14);
    auto task_records = col.records_for_task("task_1");
    assert(task_records.size() == 14);
    assert(to_string(task_records[0].phase) == "A0_AGENT_REQUEST");
    assert(to_string(task_records[13].phase) == "A13_CANCELLATION");

    std::cout << "[PASS] test_agent_telemetry_phases\n";
}

int main() {
    std::cout << "=== RUNNING TEST SUITE: test_phase7_agent_contracts ===\n";
    test_contracts_defaults();
    test_plan_state_machine_enum();
    test_agent_telemetry_phases();
    std::cout << "All agent contract tests passed successfully.\n";
    return 0;
}
