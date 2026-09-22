#include "../../runtime/policy/policy_engine.hpp"
#include "../../runtime/permissions/permission_service.hpp"
#include <cassert>
#include <iostream>

void test_policy_and_permissions() {
    // 1. Test Policy Engine
    vani::runtime::PolicyEngine policy;
    policy.set_strict_mode(true);

    // Low risk allowed
    assert(policy.evaluate({
        .actor_id = "agent.odysseus",
        .capability_id = "fs.read",
        .risk_level = vani::contracts::RiskLevel::Low,
        .is_local_execution = true,
        .is_user_present = true,
        .metadata = {}
    }) == vani::runtime::PolicyDecision::Allow);

    // High risk requires confirmation
    assert(policy.evaluate({
        .actor_id = "agent.odysseus",
        .capability_id = "terminal.exec",
        .risk_level = vani::contracts::RiskLevel::High,
        .is_local_execution = true,
        .is_user_present = true,
        .metadata = {}
    }) == vani::runtime::PolicyDecision::RequireConfirmation);

    // Restricted risk denied
    assert(policy.evaluate({
        .actor_id = "agent.odysseus",
        .capability_id = "kernel.patch",
        .risk_level = vani::contracts::RiskLevel::Restricted,
        .is_local_execution = true,
        .is_user_present = true,
        .metadata = {}
    }) == vani::runtime::PolicyDecision::Deny);

    // Untrusted plugin requires sandbox
    assert(policy.evaluate({
        .actor_id = "plugin.unverified_tool",
        .capability_id = "network.egress",
        .risk_level = vani::contracts::RiskLevel::Low,
        .is_local_execution = true,
        .is_user_present = true,
        .metadata = {}
    }) == vani::runtime::PolicyDecision::RequireSandbox);

    // 2. Test Permission Service
    vani::runtime::PermissionService permissions;
    assert(!permissions.has_permission("agent.hermes", "browser.control"));

    permissions.grant_permission("agent.hermes", "browser.control");
    assert(permissions.has_permission("agent.hermes", "browser.control"));

    permissions.revoke_permission("agent.hermes", "browser.control");
    assert(!permissions.has_permission("agent.hermes", "browser.control"));

    // Wildcard permission
    permissions.grant_permission("agent.odysseus", "filesystem.*");
    assert(permissions.has_permission("agent.odysseus", "filesystem.read"));

    std::cout << "  [PASS] test_policy_and_permissions\n";
}
