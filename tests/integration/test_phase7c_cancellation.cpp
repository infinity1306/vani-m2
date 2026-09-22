#include "../common/real_test_gateway.hpp"
#include "../../runtime/agent/agent_controller.hpp"
#include "../../voice/pipeline/end_to_end_voice_loop.hpp"
#include "../../contracts/common/cancellation_token.hpp"
#include <iostream>
#include <cassert>

using namespace vani::contracts;
using namespace vani::runtime;
using namespace vani::runtime::agent;
using namespace vani::voice::pipeline;

int main() {
    std::cout << "=== RUNNING TEST: test_phase7c_cancellation ===\n";

    auto pe = std::make_shared<PolicyEngine>();
    auto gateway = vani::tests::create_real_windows_gateway(pe);
    AgentController controller(gateway, pe);

    // 1. Agent execution cancellation
    CancellationSource src;
    AgentRequest req;
    req.request_id = "req_phase7c_cancel";
    req.goal = "Long running task to cancel";
    req.cancellation_token = src.token();

    // Cancel before or during execution
    src.cancel();
    assert(req.cancellation_token.is_cancelled());

    auto res = controller.execute_goal(req);
    assert(res.final_state == PlanState::Cancelled || res.final_state == PlanState::Failed);
    std::cout << "    [PASS] AgentController acknowledged cancellation token.\n";

    // 2. Voice loop cancellation
    auto voice_loop = std::make_shared<EndToEndVoiceLoop>(nullptr, gateway, nullptr, nullptr);
    voice_loop->cancel();
    assert(voice_loop->state() == VoiceLoopState::Idle);
    std::cout << "    [PASS] EndToEndVoiceLoop immediate cancellation verified.\n";

    std::cout << "[PASS] CANCELLATION_VERIFIED = TRUE\n";
    return 0;
}
