#include "../common/real_test_gateway.hpp"
#include "../../runtime/agent/agent_controller.hpp"
#include <iostream>
#include <cassert>

using namespace vani::contracts;
using namespace vani::runtime;
using namespace vani::runtime::agent;

int main() {
    std::cout << "=== RUNNING TEST: test_phase7b_real_toolgateway ===\n";

    auto pe = std::make_shared<PolicyEngine>();
    auto gateway = vani::tests::create_real_windows_gateway(pe);
    assert(gateway != nullptr);

    AgentController controller(gateway, pe);

    // Verify gateway pointer inside components
    assert(controller.policy_boundary() != nullptr);

    // Test a safe capability execution through real gateway
    AgentRequest req;
    req.request_id = "req_gw_test_1";
    req.goal = "notepad kholo";

    auto res = controller.execute_goal(req);
    assert(res.final_state == PlanState::Completed);
    assert(res.verified);

    // Clean up
    vani::capabilities::system::ToolExecutionPipelineContext close_ctx;
    close_ctx.capability_id = "application.close";
    close_ctx.tool_id = "app_manager.close";
    close_ctx.arguments_json = "{\"app_name\":\"notepad\"}";
    close_ctx.actor_id = "agent.planner";
    gateway->execute(close_ctx);

    std::cout << "[PASS] REAL_TOOLGATEWAY_INSTANTIATED = TRUE\n";
    std::cout << "       Gateway Status: ACTIVE (Non-null)\n";
    std::cout << "       Execution Result: " << res.final_response << "\n";
    std::cout << "ToolGateway wiring verified.\n";
    return 0;
}
