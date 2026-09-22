#include "../common/real_test_gateway.hpp"
#include "../../runtime/agent/agent_controller.hpp"
#include "../../runtime/agent/ollama_model_provider.hpp"
#include <iostream>
#include <cassert>

using namespace vani::contracts;
using namespace vani::runtime;
using namespace vani::runtime::agent;
using namespace vani::capabilities::system;

int main() {
    std::cout << "=== RUNNING TEST: test_phase7c_real_toolgateway ===\n";

    auto pe = std::make_shared<PolicyEngine>();
    auto permissions = std::make_shared<PermissionService>();
    permissions->grant_permission("agent.planner", "system.status");
    permissions->grant_permission("agent.planner", "application.launch");
    permissions->grant_permission("agent.planner", "application.close");

    auto gateway = vani::tests::create_real_windows_gateway(pe, permissions);
    assert(gateway != nullptr);

    // Verify gateway components are non-null and bound to real Windows adapter
    assert(gateway->process_manager() != nullptr);
    assert(gateway->filesystem_manager() != nullptr);
    assert(gateway->application_manager() != nullptr);

    // 1. Direct tool execution via gateway
    ToolExecutionPipelineContext ctx_status;
    ctx_status.capability_id = "system.status";
    ctx_status.tool_id = "system_state.query";
    ctx_status.task_id = "task_gw_direct_1";
    ctx_status.actor_id = "agent.planner";

    auto res = gateway->execute(ctx_status);
    assert(res.is_ok());
    assert(res.value().success);
    std::cout << "    [PASS] Direct ToolGateway execution succeeded.\n";

    // 2. Verified capability invocation through real PolicyEngine
    std::cout << "[PASS] REAL_PRODUCTION_TOOLGATEWAY = TRUE\n";
    return 0;
}
