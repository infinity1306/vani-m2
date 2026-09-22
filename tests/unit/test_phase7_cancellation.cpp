#include "../../runtime/agent/execution_scheduler.hpp"
#include "../../runtime/policy/policy_engine.hpp"
#include <iostream>
#include <cassert>
#include <thread>

using namespace vani::contracts;
using namespace vani::runtime;
using namespace vani::runtime::agent;

void test_cancellation_during_execution() {
    auto pe = std::make_shared<PolicyEngine>();
    auto boundary = std::make_shared<PolicyBoundary>(pe);
    auto verifier = std::make_shared<PostconditionVerifier>();
    ExecutionScheduler scheduler(nullptr, boundary, verifier);

    CancellationSource cancel_src;

    AgentRequest req;
    req.request_id = "req_cancel";
    req.cancellation_token = cancel_src.token();

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
    plan.plan_id = "plan_cancel";
    plan.steps = {s1, s2};

    // Trigger cancellation upon step 1 completion
    auto cb = [&](const AgentStep& s, const AgentObservation&) {
        if (s.step_id == "step_1") {
            cancel_src.cancel();
        }
    };

    auto res = scheduler.execute_plan(plan, req, cb);
    assert(res.final_state == PlanState::Cancelled);
    assert(res.error_message.find("cancelled") != std::string::npos);
    assert(res.total_steps_executed == 1);

    std::cout << "[PASS] test_cancellation_during_execution\n";
}

int main() {
    std::cout << "=== RUNNING TEST SUITE: test_phase7_cancellation ===\n";
    test_cancellation_during_execution();
    std::cout << "All cancellation tests passed successfully.\n";
    return 0;
}
