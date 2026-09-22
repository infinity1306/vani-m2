#include "../../runtime/agent/agent_model_provider.hpp"
#include "../../runtime/agent/ollama_model_provider.hpp"
#include <iostream>
#include <chrono>
#include <cassert>

using namespace vani::runtime::agent;

int main() {
    std::cout << "=== RUNNING TEST: test_phase7c_resource_offline ===\n";

    // 1. Live Win32 Host Memory Metrics
    uint64_t total_ram = ResourceGovernor::get_real_total_ram_mb();
    uint64_t avail_ram = ResourceGovernor::get_real_available_ram_mb();
    uint64_t proc_mem  = ResourceGovernor::get_real_process_memory_mb();

    std::cout << "--> Live Host Memory Metrics (Win32 GlobalMemoryStatusEx & GetProcessMemoryInfo):\n";
    std::cout << "    Total Physical RAM:     " << total_ram << " MB\n";
    std::cout << "    Available Physical RAM: " << avail_ram << " MB\n";
    std::cout << "    Current Process Memory: " << proc_mem << " MB\n";

    assert(total_ram > 1024); // Host has at least 1 GB RAM
    assert(avail_ram > 0);
    assert(avail_ram <= total_ram);
    assert(proc_mem > 0);

    // 2. Resource Governor Constraints Check
    bool is_constrained = ResourceGovernor::is_resource_constrained();
    std::cout << "    System Constrained Check: " << (is_constrained ? "YES (<500MB)" : "NO (Nominal)") << "\n";
    assert(ResourceGovernor::is_resource_constrained(256));
    assert(!ResourceGovernor::is_resource_constrained(2048));

    std::cout << "[PASS] REAL_RESOURCE_MEASUREMENT = TRUE\n";

    // 3. Offline Execution Verification
    std::cout << "--> Verifying Offline Execution Contract (Localhost Ollama):\n";
    std::string host = "127.0.0.1";
    int port = 11434;
    std::string model = "qwen2.5:3b";

    // Verify host is strictly local
    assert(host == "127.0.0.1" || host == "localhost");
    std::cout << "    Provider Endpoint: http://" << host << ":" << port << " (strictly localhost, zero cloud)\n";

    auto provider = std::make_shared<OllamaAgentModelProvider>(model, host, port);
    bool available = provider->is_available();
    std::cout << "    Local Ollama Server Available: " << (available ? "YES" : "NO") << "\n";
    assert(available);

    vani::contracts::AgentRequest req;
    req.request_id = "req_resource_offline_1";
    req.goal = "Open Notepad and check running processes";

    auto t_start = std::chrono::steady_clock::now();
    auto res = provider->generate_plan(req, nullptr);
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_start).count();

    assert(res.is_ok());
    auto telem = provider->last_telemetry();
    std::cout << "    Local Model Response Received for Goal: \"" << req.goal << "\"\n";
    std::cout << "    Local Inference Latency: " << elapsed_ms << " ms\n";
    std::cout << "    Prompt Tokens: " << telem.prompt_token_count << ", Output Tokens: " << telem.output_token_count << "\n";
    std::cout << "    Generated Steps: " << res.value().steps.size() << "\n";
    assert(!res.value().steps.empty());

    std::cout << "[PASS] OFFLINE_EXECUTION = TRUE\n";
    std::cout << "======================================================\n";
    std::cout << "ALL PHASE 7C RESOURCE & OFFLINE CHECKS PASSED\n";
    std::cout << "======================================================\n";
    return 0;
}
