#include "../../runtime/agent/agent_model_provider.hpp"
#include <iostream>
#include <cassert>

using namespace vani::runtime::agent;

int main() {
    std::cout << "=== RUNNING TEST: test_phase7b_resource_runtime ===\n";

    uint64_t total_ram = ResourceGovernor::get_real_total_ram_mb();
    uint64_t avail_ram = ResourceGovernor::get_real_available_ram_mb();
    uint64_t proc_mem  = ResourceGovernor::get_real_process_memory_mb();

    std::cout << "--> Live Host Memory Metrics (Win32 GlobalMemoryStatusEx):\n";
    std::cout << "    Total Physical RAM:     " << total_ram << " MB\n";
    std::cout << "    Available Physical RAM: " << avail_ram << " MB\n";
    std::cout << "    Current Process Memory: " << proc_mem << " MB\n";

    assert(total_ram > 1024); // Host has at least 1 GB RAM
    assert(avail_ram > 0);
    assert(avail_ram <= total_ram);

    // Verify unforced real evaluation
    bool is_constrained = ResourceGovernor::is_resource_constrained();
    std::cout << "    System Constrained? " << (is_constrained ? "YES (<500MB)" : "NO (Nominal)") << "\n";

    // Verify forced threshold check for bounded safety
    assert(ResourceGovernor::is_resource_constrained(256));
    assert(!ResourceGovernor::is_resource_constrained(2048));

    std::cout << "[PASS] REAL_RESOURCE_MEASUREMENT = TRUE\n";
    return 0;
}
