#include "../../runtime/agent/ollama_model_provider.hpp"
#include <iostream>
#include <cassert>

using namespace vani::contracts;
using namespace vani::runtime::agent;

int main() {
    std::cout << "=== RUNNING TEST: test_phase7b_real_llm_inference ===\n";

    OllamaAgentModelProvider provider("qwen2.5:3b");

    if (!provider.ping()) {
        // Fallback probe to llama3.2:1b if qwen isn't active
        provider.set_model_name("llama3.2:1b");
    }

    if (!provider.ping()) {
        std::cerr << "[BLOCKED] Local Ollama service is not reachable on 127.0.0.1:11434\n";
        return 1;
    }

    AgentRequest req;
    req.request_id = "req_llm_infer_1";
    req.goal = "Launch Notepad and open the system status overview";

    auto memory = std::make_shared<ShortTermTaskMemory>();
    auto res = provider.generate_plan(req, memory);

    assert(res.is_ok());
    auto tel = provider.last_telemetry();

    assert(tel.success);
    assert(tel.prompt_token_count > 0);
    assert(tel.output_token_count > 0);
    assert(tel.latency_ms > 0);
    assert(!tel.model.empty());
    assert(tel.provider == "Ollama");

    std::cout << "[PASS] REAL_LLM = TRUE\n";
    std::cout << "       Model: " << tel.model << "\n";
    std::cout << "       Provider: " << tel.provider << "\n";
    std::cout << "       Prompt Tokens: " << tel.prompt_token_count << "\n";
    std::cout << "       Output Tokens: " << tel.output_token_count << "\n";
    std::cout << "       Inference Latency: " << tel.latency_ms << " ms\n";
    std::cout << "All real LLM inference assertions verified.\n";
    return 0;
}
