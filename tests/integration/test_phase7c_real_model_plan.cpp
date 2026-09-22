#include "../common/real_test_gateway.hpp"
#include "../../runtime/agent/agent_controller.hpp"
#include "../../runtime/agent/ollama_model_provider.hpp"
#include "../../runtime/agent/plan_validator.hpp"
#include <iostream>
#include <cassert>

using namespace vani::contracts;
using namespace vani::runtime;
using namespace vani::runtime::agent;

int main() {
    std::cout << "=== RUNNING TEST: test_phase7c_real_model_plan ===\n";

    auto model_provider = std::make_shared<OllamaAgentModelProvider>("qwen2.5:3b");
    if (!model_provider->ping()) model_provider->set_model_name("llama3.2:1b");
    if (!model_provider->ping()) {
        std::cerr << "[BLOCKED] Local Ollama service is not reachable on 127.0.0.1:11434\n";
        return 1;
    }

    auto pe = std::make_shared<PolicyEngine>();
    auto gateway = vani::tests::create_real_windows_gateway(pe);
    AgentController controller(gateway, pe, model_provider);
    controller.set_require_real_model(true);

    AgentRequest req;
    req.request_id = "req_phase7c_plan_1";
    req.goal = "Launch Notepad and write notes to c:/Users/youri/OneDrive/Desktop/vani mark 2/temp_plan_7c.txt";

    std::cout << "--> Requesting plan generation from Real Local LLM (qwen2.5:3b)...\n";
    auto plan_res = model_provider->generate_plan(req, controller.memory());
    if (!plan_res.is_ok()) {
        std::cerr << "FAIL: Real model plan generation failed: " << plan_res.error().message << "\n";
        return 1;
    }

    AgentPlan plan = plan_res.value();
    auto tel = model_provider->last_telemetry();

    std::cout << "    [PASS] Model Plan Generated: " << plan.plan_id << "\n";
    std::cout << "           Model Name:        " << tel.model << "\n";
    std::cout << "           Prompt Tokens:     " << tel.prompt_token_count << "\n";
    std::cout << "           Output Tokens:     " << tel.output_token_count << "\n";
    std::cout << "           Inference Latency: " << tel.latency_ms << " ms\n";
    std::cout << "           Steps Generated:   " << plan.steps.size() << "\n";

    assert(tel.output_token_count > 0);
    assert(!plan.steps.empty());
    assert(plan.request_id == req.request_id);

    // Validate DAG and Schema
    PlanValidator validator;
    AgentResourceBudget budget;
    auto val_res = validator.validate_plan(plan, budget);
    if (!val_res.is_valid) {
        std::cerr << "FAIL: PlanValidator rejected model-generated plan: " << val_res.error_message << "\n";
        return 1;
    }

    std::cout << "    [PASS] PlanValidator confirmed DAG validity and parameter schemas.\n";
    std::cout << "[PASS] REAL_MODEL_PLAN = TRUE (Direct Plan Injection: NO)\n";
    return 0;
}
