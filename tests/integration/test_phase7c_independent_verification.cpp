#include "../common/real_test_gateway.hpp"
#include "../../runtime/agent/postcondition_verifier.hpp"
#include <iostream>
#include <cassert>

using namespace vani::contracts;
using namespace vani::runtime;
using namespace vani::runtime::agent;

int main() {
    std::cout << "=== RUNNING TEST: test_phase7c_independent_verification ===\n";

    auto pe = std::make_shared<PolicyEngine>();
    auto gateway = vani::tests::create_real_windows_gateway(pe);
    assert(gateway != nullptr);

    PostconditionVerifier verifier(gateway);

    // 1. Negative Test: Ghost Process Launch claim
    ToolResult fake_proc_claim;
    fake_proc_claim.success = true;
    fake_proc_claim.output_json = "{\"status\":\"ok\",\"process_id\":1234567}";

    AgentStep fake_proc_step;
    fake_proc_step.step_id = "fake_proc_step";
    fake_proc_step.capability_id = "application.launch";
    fake_proc_step.arguments["app_name"] = "ghost_phantom_proc_99999.exe";
    fake_proc_step.expected_postcondition = "process.running:ghost_phantom_proc_99999.exe";

    auto v_proc = verifier.verify(fake_proc_step, fake_proc_claim);
    assert(!v_proc.verified);
    assert(v_proc.failure_reason.find("not detected in live Windows kernel process table") != std::string::npos);
    std::cout << "    [PASS] Independent OS process query caught ghost process claim:\n"
              << "           " << v_proc.failure_reason << "\n";

    // 2. Negative Test: Ghost File Write claim
    AgentStep fake_fs_step;
    fake_fs_step.step_id = "fake_fs_step";
    fake_fs_step.capability_id = "filesystem.write";
    fake_fs_step.arguments["path"] = "c:/non_existent_path_vani_9999/ghost_file.txt";
    fake_fs_step.expected_postcondition = "file.exists:c:/non_existent_path_vani_9999/ghost_file.txt";

    auto v_fs = verifier.verify(fake_fs_step, fake_proc_claim);
    assert(!v_fs.verified);
    assert(v_fs.failure_reason.find("File does not exist on disk") != std::string::npos);
    std::cout << "    [PASS] Independent disk filesystem check caught ghost file claim:\n"
              << "           " << v_fs.failure_reason << "\n";

    std::cout << "[PASS] INDEPENDENT_VERIFICATION = TRUE (Tautology Free)\n";
    return 0;
}
