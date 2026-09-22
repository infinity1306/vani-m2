#include "../../runtime/agent/replanner.hpp"
#include <iostream>
#include <cassert>

using namespace vani::contracts;
using namespace vani::runtime::agent;

void test_bounded_replanning_limit() {
    Replanner replanner;

    AgentStep s;
    s.step_id = "step_fail";
    s.capability_id = "application.launch";
    s.arguments["app_name"] = "Visual Studio Code";

    AgentPlan plan;
    plan.plan_id = "plan_orig";
    plan.steps = {s};

    AgentObservation failure_obs;
    failure_obs.step_id = "step_fail";
    failure_obs.success = false;
    failure_obs.error = "Process launch failed";

    uint32_t max_replans = 3;

    // Replan 1 (current = 0)
    auto res1 = replanner.replan(plan, "step_fail", failure_obs, 0, max_replans);
    assert(res1.success);
    assert(res1.new_plan.plan_id.find("replan1") != std::string::npos);

    // Replan 2 (current = 1)
    auto res2 = replanner.replan(plan, "step_fail", failure_obs, 1, max_replans);
    assert(res2.success);

    // Replan 3 (current = 2)
    auto res3 = replanner.replan(plan, "step_fail", failure_obs, 2, max_replans);
    assert(res3.success);

    // Replan 4 (current = 3, at limit) -> MUST FAIL
    auto res4 = replanner.replan(plan, "step_fail", failure_obs, 3, max_replans);
    assert(!res4.success);
    assert(res4.error_message.find("PLAN_FAILED") != std::string::npos);

    std::cout << "[PASS] test_bounded_replanning_limit\n";
}

void test_ambiguous_target_clarification() {
    Replanner replanner;
    AgentPlan plan;
    plan.plan_id = "plan_ambig";

    // Test 8: Ambiguity: Multiple project candidates found
    std::vector<std::string> candidates = {
        "c:/Users/youri/projects/vani_mark_1",
        "c:/Users/youri/projects/vani_mark_2"
    };

    auto ambig_res = replanner.handle_ambiguity(plan, "step_locate", candidates);
    assert(!ambig_res.success);
    assert(ambig_res.is_ambiguous);
    assert(ambig_res.ambiguous_candidates.size() == 2);
    assert(ambig_res.error_message.find("AMBIGUOUS_TARGET") != std::string::npos);
    assert(ambig_res.error_message.find("User clarification required") != std::string::npos);

    std::cout << "[PASS] test_ambiguous_target_clarification\n";
}

int main() {
    std::cout << "=== RUNNING TEST SUITE: test_phase7_replanning ===\n";
    test_bounded_replanning_limit();
    test_ambiguous_target_clarification();
    std::cout << "All replanning tests passed successfully.\n";
    return 0;
}
