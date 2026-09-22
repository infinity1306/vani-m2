#include "../../runtime/agent/execution_scheduler.hpp"
#include "../../runtime/policy/policy_engine.hpp"
#include <iostream>
#include <cassert>

using namespace vani::contracts;
using namespace vani::runtime;
using namespace vani::runtime::agent;

void test_sequential_dependency_execution() {
    auto pe = std::make_shared<PolicyEngine>();
    auto boundary = std::make_shared<PolicyBoundary>(pe);
    auto verifier = std::make_shared<PostconditionVerifier>();
    ExecutionScheduler scheduler(nullptr, boundary, verifier);

    AgentRequest req;
    req.request_id = "req_dep";

    AgentStep s1;
    s1.step_id = "step_1";
    s1.capability_id = "system.status";
    s1.expected_postcondition = "system.queried";
    s1.timeout_ms = 5000;

    AgentStep s2;
    s2.step_id = "step_2";
    s2.capability_id = "system.status";
    s2.dependencies.push_back("step_1");
    s2.expected_postcondition = "system.queried";
    s2.timeout_ms = 5000;

    AgentPlan plan;
    plan.plan_id = "plan_dep";
    plan.steps = {s1, s2};

    std::vector<std::string> execution_order;
    auto cb = [&](const AgentStep& s, const AgentObservation&) {
        execution_order.push_back(s.step_id);
    };

    auto res = scheduler.execute_plan(plan, req, cb);
    assert(res.final_state == PlanState::Completed);
    assert(res.verified);
    assert(execution_order.size() == 2);
    assert(execution_order[0] == "step_1");
    assert(execution_order[1] == "step_2");

    std::cout << "[PASS] test_sequential_dependency_execution\n";
}

void test_failure_propagation_blocks_dependents() {
    auto pe = std::make_shared<PolicyEngine>();
    auto boundary = std::make_shared<PolicyBoundary>(pe);
    auto verifier = std::make_shared<PostconditionVerifier>();
    // Force verification failure on step 1
    verifier->set_force_verification_failure(true);

    ExecutionScheduler scheduler(nullptr, boundary, verifier);

    AgentRequest req;
    req.request_id = "req_fail_prop";

    AgentStep s1;
    s1.step_id = "step_1";
    s1.capability_id = "application.launch";
    s1.expected_postcondition = "process.running";
    s1.retry_policy_max_attempts = 1;

    AgentStep s2;
    s2.step_id = "step_2";
    s2.capability_id = "browser.open_url";
    s2.dependencies.push_back("step_1");
    s2.expected_postcondition = "browser.navigated";

    AgentPlan plan;
    plan.plan_id = "plan_fail";
    plan.steps = {s1, s2};

    std::vector<std::string> executed_steps;
    auto cb = [&](const AgentStep& s, const AgentObservation&) {
        executed_steps.push_back(s.step_id);
    };

    auto res = scheduler.execute_plan(plan, req, cb);
    assert(res.final_state == PlanState::Failed);
    assert(executed_steps.size() == 1);
    assert(executed_steps[0] == "step_1");

    // Check that step_2 was blocked and not executed
    bool step2_executed = false;
    for (const auto& s : executed_steps) {
        if (s == "step_2") step2_executed = true;
    }
    assert(!step2_executed);

    std::cout << "[PASS] test_failure_propagation_blocks_dependents\n";
}

void test_bounded_transient_retry() {
    auto pe = std::make_shared<PolicyEngine>();
    auto boundary = std::make_shared<PolicyBoundary>(pe);
    auto verifier = std::make_shared<PostconditionVerifier>();
    ExecutionScheduler scheduler(nullptr, boundary, verifier);

    // Test 5: Simulate 1 transient failure before recovery
    scheduler.set_simulated_transient_failures(1);

    AgentRequest req;
    req.request_id = "req_retry";

    AgentStep s;
    s.step_id = "step_retry";
    s.capability_id = "system.status";
    s.retry_policy_max_attempts = 2; // Allows 2 attempts
    s.timeout_ms = 5000;

    AgentPlan plan;
    plan.plan_id = "plan_retry";
    plan.steps = {s};

    auto res = scheduler.execute_plan(plan, req);
    assert(res.final_state == PlanState::Completed);
    assert(res.total_retries == 1);
    assert(res.total_tool_calls == 2);
    assert(res.observations.size() == 1);
    assert(res.observations[0].attempts_taken == 2);

    std::cout << "[PASS] test_bounded_transient_retry\n";
}

int main() {
    std::cout << "=== RUNNING TEST SUITE: test_phase7_scheduler ===\n";
    test_sequential_dependency_execution();
    test_failure_propagation_blocks_dependents();
    test_bounded_transient_retry();
    std::cout << "All scheduler tests passed successfully.\n";
    return 0;
}
