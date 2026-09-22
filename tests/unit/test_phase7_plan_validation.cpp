#include "../../runtime/agent/plan_validator.hpp"
#include <iostream>
#include <cassert>

using namespace vani::contracts;
using namespace vani::runtime::agent;

void test_valid_plan() {
    PlanValidator validator;
    AgentResourceBudget budget;

    AgentStep s1;
    s1.step_id = "step_1";
    s1.capability_id = "application.launch";
    s1.tool_id = "app_manager.launch";
    s1.arguments["app_name"] = "Google Chrome";
    s1.expected_postcondition = "process.running:chrome.exe";
    s1.timeout_ms = 10000;
    s1.retry_policy_max_attempts = 2;

    AgentStep s2;
    s2.step_id = "step_2";
    s2.capability_id = "browser.open_url";
    s2.tool_id = "browser_manager.open";
    s2.arguments["url"] = "https://google.com";
    s2.dependencies.push_back("step_1");
    s2.timeout_ms = 10000;
    s2.retry_policy_max_attempts = 2;

    AgentPlan plan;
    plan.plan_id = "plan_ok";
    plan.steps = {s1, s2};

    auto res = validator.validate_plan(plan, budget);
    assert(res.is_valid);
    assert(res.validation_errors.empty());
    std::cout << "[PASS] test_valid_plan\n";
}

void test_empty_and_budget_exceeded() {
    PlanValidator validator;
    AgentResourceBudget budget;
    budget.max_steps = 3;

    // Empty plan
    AgentPlan empty_plan;
    auto res_empty = validator.validate_plan(empty_plan, budget);
    assert(!res_empty.is_valid);

    // Exceed max_steps
    AgentPlan large_plan;
    for (int i = 0; i < 5; ++i) {
        AgentStep s;
        s.step_id = "step_" + std::to_string(i);
        s.capability_id = "system.status";
        s.timeout_ms = 5000;
        s.retry_policy_max_attempts = 1;
        large_plan.steps.push_back(s);
    }
    auto res_large = validator.validate_plan(large_plan, budget);
    assert(!res_large.is_valid);
    assert(res_large.error_message.find("exceeds maximum budget") != std::string::npos);

    std::cout << "[PASS] test_empty_and_budget_exceeded\n";
}

void test_cyclic_plan_rejected() {
    PlanValidator validator;
    AgentResourceBudget budget;

    AgentStep s1;
    s1.step_id = "step_1";
    s1.capability_id = "system.status";
    s1.dependencies.push_back("step_2"); // Cycle: 1 -> 2 -> 1
    s1.timeout_ms = 5000;
    s1.retry_policy_max_attempts = 1;

    AgentStep s2;
    s2.step_id = "step_2";
    s2.capability_id = "system.status";
    s2.dependencies.push_back("step_1");
    s2.timeout_ms = 5000;
    s2.retry_policy_max_attempts = 1;

    AgentPlan cyclic_plan;
    cyclic_plan.steps = {s1, s2};

    auto res = validator.validate_plan(cyclic_plan, budget);
    assert(!res.is_valid);
    bool found_cycle_err = false;
    for (const auto& err : res.validation_errors) {
        if (err.find("Cyclic dependency") != std::string::npos) found_cycle_err = true;
    }
    assert(found_cycle_err);

    std::cout << "[PASS] test_cyclic_plan_rejected\n";
}

void test_tool_injection_and_adversarial_rejected() {
    PlanValidator validator;
    AgentResourceBudget budget;

    // Test 12: Tool Injection / Malicious shell pattern
    AgentStep s_inj;
    s_inj.step_id = "step_inj";
    s_inj.capability_id = "application.launch";
    s_inj.arguments["app_name"] = "chrome; rm -rf /";
    s_inj.expected_postcondition = "process.running";
    s_inj.timeout_ms = 5000;

    AgentPlan p_inj;
    p_inj.steps = {s_inj};
    auto res_inj = validator.validate_plan(p_inj, budget);
    assert(!res_inj.is_valid);
    bool found_inj_err = false;
    for (const auto& err : res_inj.validation_errors) {
        if (err.find("injection") != std::string::npos || err.find("Disallowed") != std::string::npos) {
            found_inj_err = true;
        }
    }
    assert(found_inj_err);

    // Adversarial: Planner attempts to override policy
    AgentStep s_pol;
    s_pol.step_id = "step_pol";
    s_pol.capability_id = "system.status";
    s_pol.arguments["override_policy"] = "true";
    s_pol.timeout_ms = 5000;

    AgentPlan p_pol;
    p_pol.steps = {s_pol};
    auto res_pol = validator.validate_plan(p_pol, budget);
    assert(!res_pol.is_valid);

    // Adversarial: Unknown tool or capability
    AgentStep s_unk;
    s_unk.step_id = "step_unk";
    s_unk.capability_id = "unauthorized.privileged_kernel_hack";
    s_unk.timeout_ms = 5000;

    AgentPlan p_unk;
    p_unk.steps = {s_unk};
    auto res_unk = validator.validate_plan(p_unk, budget);
    assert(!res_unk.is_valid);

    // Adversarial: Unbounded retry policy
    AgentStep s_unb;
    s_unb.step_id = "step_unb";
    s_unb.capability_id = "system.status";
    s_unb.retry_policy_max_attempts = 999; // Exceeds budget.max_retries_per_step
    s_unb.timeout_ms = 5000;

    AgentPlan p_unb;
    p_unb.steps = {s_unb};
    auto res_unb = validator.validate_plan(p_unb, budget);
    assert(!res_unb.is_valid);

    std::cout << "[PASS] test_tool_injection_and_adversarial_rejected\n";
}

int main() {
    std::cout << "=== RUNNING TEST SUITE: test_phase7_plan_validation ===\n";
    test_valid_plan();
    test_empty_and_budget_exceeded();
    test_cyclic_plan_rejected();
    test_tool_injection_and_adversarial_rejected();
    std::cout << "All plan validation tests passed successfully.\n";
    return 0;
}
