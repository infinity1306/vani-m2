#include "../common/real_test_gateway.hpp"
#include "../../voice/pipeline/end_to_end_voice_loop.hpp"
#include "../../runtime/agent/agent_controller.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

using namespace vani::contracts;
using namespace vani::voice::pipeline;
using namespace vani::runtime;
using namespace vani::runtime::agent;

int main() {
    std::cout << "=== RUNNING TEST: test_phase7b_voice_to_agent ===\n";

    auto pe = std::make_shared<PolicyEngine>();
    auto gateway = vani::tests::create_real_windows_gateway(pe);
    auto agent_ctrl = std::make_shared<AgentController>(gateway, pe);

    // Instantiate EndToEndVoiceLoop
    auto voice_loop = std::make_shared<EndToEndVoiceLoop>(nullptr, gateway, nullptr, nullptr);

    // CRITICAL REQUIREMENT: Call set_agent_controller and prove it is active
    voice_loop->set_agent_controller(agent_ctrl);
    std::cout << "--> set_agent_controller() successfully invoked on EndToEndVoiceLoop\n";

    // Simulate an intent turn that routes to the Agent Path
    GatedTurnResult turn;
    turn.turn_id = "voice_turn_agent_1";
    turn.wake_triggered = true;
    turn.wake_phrase = "vani";
    turn.raw_transcript = "Chrome kholo aur Google open karo";
    turn.normalized_text = "Chrome kholo aur Google open karo";
    turn.detected_intent = "application.launch";

    bool turn_completed = false;
    voice_loop->set_turn_callback([&](const VoiceLoopTurnEvidence& ev) {
        turn_completed = true;
        std::cout << "    Turn Completed Callback invoked. Intent: " << ev.detected_intent << "\n";
    });

    std::cout << "--> Dispatching compound voice turn to voice loop...\n";
    voice_loop->handle_intent_ready(turn);
    assert(turn_completed);

    // Verify agent was invoked and completed
    auto entries = agent_ctrl->audit_journal()->entries_for_request("voice_turn_agent_1");
    assert(!entries.empty());
    std::cout << "    Agent Audit Journal recorded " << entries.size() << " steps for voice turn!\n";

    std::cout << "[PASS] REAL_VOICE_TO_AGENT = TRUE\n";
    return 0;
}
