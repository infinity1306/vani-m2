#include "../common/real_test_gateway.hpp"
#include "../../runtime/agent/agent_controller.hpp"
#include "../../runtime/agent/ollama_model_provider.hpp"
#include "../../runtime/agent/postcondition_verifier.hpp"
#include <iostream>
#include <filesystem>
#include <cassert>
#include <functional>

using namespace vani::contracts;
using namespace vani::runtime;
using namespace vani::runtime::agent;
using namespace vani::capabilities::system;

// Helper to compute a structural signature hash of an AgentPlan
static size_t compute_plan_hash(const AgentPlan& plan) {
    std::string sig;
    for (const auto& s : plan.steps) {
        sig += s.step_id + ":" + s.capability_id + ":" + s.tool_id;
        for (const auto& [k, v] : s.arguments) {
            sig += "[" + k + "=" + v + "]";
        }
        sig += ";";
    }
    return std::hash<std::string>{}(sig);
}

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    std::cout << "=== RUNNING TEST: test_phase7c_agentcontroller_real_replan ===" << std::endl;

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
    controller.set_require_real_model(true);

    std::string goal = "Launch custom_missing_editor_tool_xyz.exe or fallback to notepad.exe";

    AgentRequest req;
    req.request_id = "req_phase7c_replan_1";
    req.goal = goal;
    req.resource_budget.max_replans = 2;

    std::cout << "--> Submitting goal with intentional secondary failure to AgentController:\n"
              << "    Goal: " << goal << "\n";

    auto res = controller.execute_goal(req);

    std::cout << "--> Execution Completed:\n";
    std::cout << "    Final State:         " << to_string(res.final_state) << "\n";
    std::cout << "    Total Steps Done:    " << res.total_steps_executed << "\n";
    std::cout << "    Total Replans:       " << res.total_replans << "\n";
    std::cout << "    Final Response:      " << res.final_response << "\n";

    AgentPlan plan_a = controller.last_initial_plan();
    AgentPlan plan_b = controller.last_replanned_plan();

    std::cout << "\n--> FORENSIC COMPARISON: Plan A vs Plan B\n";
    std::cout << "    Plan A ID:           " << plan_a.plan_id << " (Steps: " << plan_a.steps.size() << ")\n";
    for (size_t i = 0; i < plan_a.steps.size(); ++i) {
        std::cout << "      Plan A Step [" << i + 1 << "]: " << plan_a.steps[i].step_id
                  << " -> " << plan_a.steps[i].capability_id;
        if (plan_a.steps[i].arguments.count("app_name")) {
            std::cout << " (app_name=" << plan_a.steps[i].arguments.at("app_name") << ")";
        }
        std::cout << "\n";
    }

    std::cout << "    Plan B ID:           " << plan_b.plan_id << " (Steps: " << plan_b.steps.size() << ")\n";
    for (size_t i = 0; i < plan_b.steps.size(); ++i) {
        std::cout << "      Plan B Step [" << i + 1 << "]: " << plan_b.steps[i].step_id
                  << " -> " << plan_b.steps[i].capability_id;
        if (plan_b.steps[i].arguments.count("app_name")) {
            std::cout << " (app_name=" << plan_b.steps[i].arguments.at("app_name") << ")";
        }
        std::cout << "\n";
    }

    size_t hash_a = compute_plan_hash(plan_a);
    size_t hash_b = compute_plan_hash(plan_b);
    std::cout << "    Plan A Hash:         " << hash_a << "\n";
    std::cout << "    Plan B Hash:         " << hash_b << "\n";

    // Rigorous assertions
    assert(res.total_replans >= 1);
    assert(res.final_state == PlanState::Completed);
    assert(!plan_a.steps.empty());
    assert(!plan_b.steps.empty());
    assert(hash_a != hash_b); // Plan B MUST be structurally distinct from Plan A!

    // Verify Notepad process was launched during Plan B recovery
    auto procs = gateway->process_manager()->list_processes();
    bool notepad_running = false;
    if (procs.is_success()) {
        for (const auto& p : procs.value()) {
            if (p.name.find("notepad") != std::string::npos || p.name.find("Notepad") != std::string::npos) {
                notepad_running = true;
                std::cout << "    [VERIFIED] Plan B live process confirmed in OS process table: " 
                          << p.name << " (PID " << p.pid << ")\n";
                break;
            }
        }
    }
    assert(notepad_running);

    // Clean up OS resources
    ToolExecutionPipelineContext ctx_close;
    ctx_close.capability_id = "application.close";
    ctx_close.tool_id = "app_manager.close";
    ctx_close.task_id = "task_replan_clean";
    ctx_close.actor_id = "agent.planner";
    ctx_close.arguments_json = "{\"app_name\":\"notepad\"}";
    gateway->execute(ctx_close);


    std::cout << "[PASS] AGENTCONTROLLER_REAL_REPLAN = TRUE\n";
    std::cout << "       PLAN_A_SOURCE = REAL_MODEL\n";
    std::cout << "       PLAN_B_SOURCE = REAL_MODEL\n";
    std::cout << "       DETERMINISTIC_REPLANNER = BYPASSED (Real LLM Used)\n";
    return 0;
}
