#include "../common/real_test_gateway.hpp"
#include "../../runtime/agent/postcondition_verifier.hpp"
#include <iostream>
#include <cassert>

using namespace vani::contracts;
using namespace vani::runtime;
using namespace vani::runtime::agent;

int main() {
    std::cout << "=== RUNNING TEST: test_phase7b_independent_verification ===\n";

    auto pe = std::make_shared<PolicyEngine>();
    auto gateway = vani::tests::create_real_windows_gateway(pe);
    assert(gateway != nullptr);

    PostconditionVerifier verifier(gateway);

    // 1. Tool claims success for process launch, but process does NOT exist in live OS table
    ToolResult fake_tool_success;
    fake_tool_success.success = true;
    fake_tool_success.output_json = "{\"status\":\"ok\",\"process_id\":999999}";

    AgentStep fake_step_launch;
    fake_step_launch.step_id = "fake_launch";
    fake_step_launch.capability_id = "application.launch";
    fake_step_launch.arguments["app_name"] = "definitely_non_existent_process_abc123456.exe";
    fake_step_launch.expected_postcondition = "process.running:definitely_non_existent_process_abc123456.exe";

    auto v_res1 = verifier.verify(fake_step_launch, fake_tool_success);
    // MUST FAIL because it does not exist in live Win32 process table!
    assert(!v_res1.verified);
    assert(v_res1.failure_reason.find("not detected in live Windows kernel process table") != std::string::npos);
    std::cout << "[PASS] Ghost process successfully rejected by independent OS process query:\n"
              << "       " << v_res1.failure_reason << "\n";

    // 2. Tool claims success for filesystem write, but file does NOT exist on disk
    AgentStep fake_step_fs;
    fake_step_fs.step_id = "fake_fs";
    fake_step_fs.capability_id = "filesystem.write";
    fake_step_fs.arguments["path"] = "c:/non_existent_vani_ghost_directory_12345/missing_file.txt";
    fake_step_fs.expected_postcondition = "file.exists:c:/non_existent_vani_ghost_directory_12345/missing_file.txt";

    auto v_res2 = verifier.verify(fake_step_fs, fake_tool_success);
    // MUST FAIL because file is not on physical disk!
    assert(!v_res2.verified);
    assert(v_res2.failure_reason.find("File does not exist on disk") != std::string::npos);
    std::cout << "[PASS] Ghost file successfully rejected by independent disk filesystem check:\n"
              << "       " << v_res2.failure_reason << "\n";

    std::cout << "[PASS] INDEPENDENT_VERIFICATION_VALIDATED = TRUE\n";
    return 0;
}
