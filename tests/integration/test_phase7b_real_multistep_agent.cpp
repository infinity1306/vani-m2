#include "../common/real_test_gateway.hpp"
#include "../../runtime/agent/agent_controller.hpp"
#include "../../runtime/agent/ollama_model_provider.hpp"
#include <iostream>
#include <filesystem>
#include <cassert>
#include <thread>
#include <chrono>

using namespace vani::contracts;
using namespace vani::runtime;
using namespace vani::runtime::agent;
using namespace vani::capabilities::system;

int main() {
    std::cout << "=== RUNNING TEST: test_phase7b_real_multistep_agent ===\n";

    auto model_provider = std::make_shared<OllamaAgentModelProvider>("qwen2.5:3b");
    if (!model_provider->ping()) model_provider->set_model_name("llama3.2:1b");
    if (!model_provider->ping()) {
        std::cerr << "[BLOCKED] Local Ollama service is not reachable on 127.0.0.1:11434\n";
        return 1;
    }

    auto pe = std::make_shared<PolicyEngine>();
    auto gateway = vani::tests::create_real_windows_gateway(pe);
    assert(gateway != nullptr);

    AgentController controller(gateway, pe, model_provider);

    std::string test_file = "c:/Users/youri/OneDrive/Desktop/vani mark 2/temp_multistep_test.txt";
    std::string goal = "Write 'VANI Phase 7B MultiStep Active' to " + test_file + " and launch Notepad";

    AgentRequest req;
    req.request_id = "req_multistep_real_1";
    req.goal = goal;

    std::cout << "--> Submitting goal to Agent with Real Local LLM: " << goal << "\n";
    auto res = controller.execute_goal(req);

    auto tel = model_provider->last_telemetry();
    std::cout << "--> Model Plan Generated: " << res.plan_id << "\n";
    std::cout << "    Tokens Generated: " << tel.output_token_count << "\n";
    std::cout << "    Inference Latency: " << tel.latency_ms << " ms\n";
    std::cout << "    Final State: " << static_cast<int>(res.final_state) << "\n";
    std::cout << "    Error Message: " << res.error_message << "\n";
    std::cout << "    Final Response: " << res.final_response << "\n";
    std::cout << "    Steps Planned & Executed: " << res.total_steps_executed << "\n";

    if (res.final_state != PlanState::Completed) {
        std::cerr << "FAIL: final_state != Completed. Error: " << res.error_message << "\n";
        return 1;
    }
    if (!res.verified) {
        std::cerr << "FAIL: Plan was not verified on system.\n";
        return 1;
    }
    if (res.total_steps_executed < 2) {
        std::cerr << "FAIL: Less than 2 steps executed: " << res.total_steps_executed << "\n";
        return 1;
    }

    // Verify independent observations
    bool saw_real_fs = false;
    bool saw_real_proc = false;

    for (const auto& obs : res.observations) {
        std::cout << "    Observation [" << obs.step_id << "]: "
                  << (obs.success ? "SUCCESS" : "FAILURE")
                  << " - " << obs.evidence << "\n";
        if (obs.evidence.find("File confirmed on physical disk") != std::string::npos) saw_real_fs = true;
        if (obs.evidence.find("Live process confirmed in Windows Process Table") != std::string::npos) saw_real_proc = true;
    }

    if (!saw_real_fs || !saw_real_proc) {
        std::cerr << "FAIL: Did not observe both real FS and real process in OS table! saw_real_fs="
                  << saw_real_fs << ", saw_real_proc=" << saw_real_proc << "\n";
        return 1;
    }

    // Clean up real OS resources
    vani::capabilities::system::ToolExecutionPipelineContext ctx_close;
    ctx_close.capability_id = "application.close";
    ctx_close.tool_id = "app_manager.close";
    ctx_close.task_id = "task_cleanup";
    ctx_close.actor_id = "agent.planner";
    ctx_close.arguments_json = "{\"app_name\":\"notepad\"}";
    gateway->execute(ctx_close);

    std::error_code ec;
    std::filesystem::remove(test_file, ec);

    std::cout << "[PASS] REAL_MULTISTEP_AGENT_EXECUTION = TRUE\n";
    return 0;
}
