#include "../../runtime/agent/execution_scheduler.hpp"
#include "../../runtime/policy/policy_engine.hpp"
#include <iostream>
#include <cassert>
#include <thread>

using namespace vani::contracts;
using namespace vani::runtime;
using namespace vani::runtime::agent;

void test_global_timeout_enforcement() {
    auto pe = std::make_shared<PolicyEngine>();
    auto boundary = std::make_shared<PolicyBoundary>(pe);
    auto verifier = std::make_shared<PostconditionVerifier>();
    ExecutionScheduler scheduler(nullptr, boundary, verifier);

    AgentRequest req;
    req.request_id = "req_timeout";
    // Set a very tight timeout of 30 ms
    req.resource_budget.global_timeout_ms = 30;

    AgentStep s1;
    s1.step_id = "step_1";
    s1.capability_id = "system.status";
    s1.timeout_ms = 5000;

    AgentStep s2;
    s2.step_id = "step_2";
    s2.capability_id = "system.status";
    s2.dependencies.push_back("step_1");
    s2.timeout_ms = 5000;

    AgentPlan plan;
    plan.plan_id = "plan_timeout";
    plan.steps = {s1, s2};

    // Inject a sleep in step 1 callback to force timeout before step 2
    auto cb = [&](const AgentStep&, const AgentObservation&) {
        std::this_thread::sleep_for(std::chrono::milliseconds(45));
    };

    auto res = scheduler.execute_plan(plan, req, cb);
    assert(res.final_state == PlanState::Timeout);
    assert(res.error_message.find("timeout") != std::string::npos);

    std::cout << "[PASS] test_global_timeout_enforcement\n";
}

int main() {
    std::cout << "=== RUNNING TEST SUITE: test_phase7_timeout ===\n";
    test_global_timeout_enforcement();
    std::cout << "All timeout tests passed successfully.\n";
    return 0;
}
