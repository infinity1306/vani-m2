#include "../../runtime/agent/ollama_model_provider.hpp"
#include "../../runtime/agent/plan_validator.hpp"
#include <iostream>
#include <cassert>

using namespace vani::contracts;
using namespace vani::runtime::agent;

int main() {
    std::cout << "=== RUNNING TEST: test_phase7b_structured_plan_generation ===\n";

    OllamaAgentModelProvider provider("qwen2.5:3b");
    if (!provider.ping()) provider.set_model_name("llama3.2:1b");
    if (!provider.ping()) {
        std::cerr << "[BLOCKED] Local Ollama service is not reachable on 127.0.0.1:11434\n";
        return 1;
    }

    AgentRequest req;
    req.request_id = "req_plan_gen_1";
    req.goal = "Launch Notepad and open browser to https://google.com";

    auto memory = std::make_shared<ShortTermTaskMemory>();
    auto res = provider.generate_plan(req, memory);
    assert(res.is_ok());

    AgentPlan plan = res.value();
    assert(!plan.steps.empty());
    assert(plan.request_id == req.request_id);

    // Validate using PlanValidator
    PlanValidator validator;
    AgentResourceBudget budget;
    auto val_res = validator.validate_plan(plan, budget);
    assert(val_res.is_valid);
    assert(val_res.validation_errors.empty());

    std::cout << "[PASS] STRUCTURED_MODEL_PLAN_GENERATED = TRUE\n";
    std::cout << "       Plan ID: " << plan.plan_id << "\n";
    std::cout << "       Step Count: " << plan.steps.size() << "\n";
    for (size_t i = 0; i < plan.steps.size(); ++i) {
        std::cout << "       Step [" << i + 1 << "]: " << plan.steps[i].step_id
                  << " -> " << plan.steps[i].capability_id
                  << " (Postcondition: " << plan.steps[i].expected_postcondition << ")\n";
    }
    std::cout << "Structured plan schema and DAG validation passed.\n";
    return 0;
}
