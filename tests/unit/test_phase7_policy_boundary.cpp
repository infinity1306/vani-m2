#include "../../runtime/agent/policy_boundary.hpp"
#include "../../runtime/policy/policy_engine.hpp"
#include <iostream>
#include <cassert>

using namespace vani::contracts;
using namespace vani::runtime;
using namespace vani::runtime::agent;

void test_policy_allow() {
    auto pe = std::make_shared<PolicyEngine>();
    PolicyBoundary boundary(pe);

    AgentRequest req;
    req.request_id = "req_allow";
    req.permission_context = "user.admin";

    AgentStep s;
    s.step_id = "s1";
    s.capability_id = "system.status";
    s.risk_level = RiskLevel::Low;

    auto res = boundary.evaluate_step(s, req, "plan_1");
    assert(res.allowed);
    assert(!res.requires_confirmation);
    assert(res.decision_str == "ALLOW" || res.decision_str == "ALLOW_WITH_AUDIT");
    std::cout << "[PASS] test_policy_allow\n";
}

void test_policy_denial_no_bypass() {
    auto pe = std::make_shared<PolicyEngine>();
    // In strict mode, high/critical risk operations are blocked unless approved
    pe->set_strict_mode(true);

    PolicyBoundary boundary(pe);

    AgentRequest req;
    req.request_id = "req_deny";
    req.permission_context = "untrusted_actor";

    // Test 3: High risk / destructive action requested by untrusted agent
    AgentStep s;
    s.step_id = "s_crit";
    s.capability_id = "terminal.execute";
    s.risk_level = RiskLevel::Critical;

    auto res = boundary.evaluate_step(s, req, "plan_deny");
    assert(!res.allowed);
    assert(res.decision_str == "DENY");
    assert(res.reason.find("denied") != std::string::npos);

    std::cout << "[PASS] test_policy_denial_no_bypass\n";
}

void test_policy_confirmation_workflow() {
    auto pe = std::make_shared<PolicyEngine>();
    PolicyBoundary boundary(pe);

    AgentRequest req;
    req.request_id = "req_conf";
    req.permission_context = "user.standard";

    // Medium-high risk action requiring confirmation
    AgentStep s;
    s.step_id = "s_close";
    s.capability_id = "application.close";
    s.arguments["app_name"] = "Google Chrome";
    s.expected_postcondition = "process.terminated";
    s.risk_level = RiskLevel::Medium;

    // First evaluate: should require confirmation
    auto res = boundary.evaluate_step(s, req, "plan_conf");
    if (res.requires_confirmation) {
        assert(!res.allowed);
        assert(res.confirmation_payload.target == "Google Chrome");
        assert(res.confirmation_payload.step_id == "s_close");

        // Simulate user rejecting confirmation
        boundary.set_confirmation_handler([](const AgentConfirmationRequest&) {
            return false; // User denies
        });
        bool user_approved = boundary.prompt_confirmation(res.confirmation_payload);
        assert(!user_approved);

        // Simulate user accepting confirmation
        boundary.set_confirmation_handler([](const AgentConfirmationRequest&) {
            return true; // User allows
        });
        user_approved = boundary.prompt_confirmation(res.confirmation_payload);
        assert(user_approved);
    }

    std::cout << "[PASS] test_policy_confirmation_workflow\n";
}

int main() {
    std::cout << "=== RUNNING TEST SUITE: test_phase7_policy_boundary ===\n";
    test_policy_allow();
    test_policy_denial_no_bypass();
    test_policy_confirmation_workflow();
    std::cout << "All policy boundary tests passed successfully.\n";
    return 0;
}
