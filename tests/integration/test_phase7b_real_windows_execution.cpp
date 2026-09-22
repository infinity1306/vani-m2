#include "../common/real_test_gateway.hpp"
#include "../../runtime/agent/postcondition_verifier.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cassert>
#include <thread>
#include <chrono>

using namespace vani::contracts;
using namespace vani::runtime;
using namespace vani::runtime::agent;
using namespace vani::capabilities::system;

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    std::cout << "=== RUNNING TEST: test_phase7b_real_windows_execution ===\n";

    try {
        auto pe = std::make_shared<PolicyEngine>();
        auto gateway = vani::tests::create_real_windows_gateway(pe);
        assert(gateway != nullptr);

        PostconditionVerifier verifier(gateway);

    // -------------------------------------------------------------
    // Scenario 1: Safe Real Process Execution & Independent Verification
    // -------------------------------------------------------------
    std::cout << "--> Executing Real Windows Process Launch: notepad.exe\n";
    vani::capabilities::system::ToolExecutionPipelineContext ctx_launch;
    ctx_launch.capability_id = "application.launch";
    ctx_launch.tool_id = "app_manager.launch";
    ctx_launch.task_id = "task_real_proc";
    ctx_launch.actor_id = "agent.planner";
    ctx_launch.arguments_json = "{\"app_name\":\"notepad.exe\"}";

    auto launch_res = gateway->execute(ctx_launch);
    assert(launch_res.is_ok());
    assert(launch_res.value().success);

    // Allow process to register in OS table
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Independent Verification of Process Launch
    AgentStep step_launch;
    step_launch.step_id = "step_proc_launch";
    step_launch.capability_id = "application.launch";
    step_launch.arguments["app_name"] = "notepad";
    step_launch.expected_postcondition = "process.running:notepad.exe";

    auto v_launch = verifier.verify(step_launch, launch_res.value());
    assert(v_launch.verified);
    assert(v_launch.evidence.find("REAL_OS_OBSERVATION") != std::string::npos);
    std::cout << "    [VERIFIED] " << v_launch.evidence << "\n";

    // Terminate the process cleanly
    std::cout << "--> Terminating Real Windows Process: notepad.exe\n";
    vani::capabilities::system::ToolExecutionPipelineContext ctx_close;
    ctx_close.capability_id = "application.close";
    ctx_close.tool_id = "app_manager.close";
    ctx_close.task_id = "task_real_proc_close";
    ctx_close.actor_id = "agent.planner";
    ctx_close.arguments_json = "{\"app_name\":\"notepad\"}";

    auto close_res = gateway->execute(ctx_close);
    assert(close_res.is_ok());

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Independent Verification of Process Termination
    AgentStep step_close;
    step_close.step_id = "step_proc_close";
    step_close.capability_id = "application.close";
    step_close.arguments["app_name"] = "notepad";
    step_close.expected_postcondition = "process.terminated:notepad";

    auto v_close = verifier.verify(step_close, close_res.value());
    assert(v_close.verified);
    assert(v_close.evidence.find("absent from live Windows Kernel Process Table") != std::string::npos);
    std::cout << "    [VERIFIED] " << v_close.evidence << "\n";

    // -------------------------------------------------------------
    // Scenario 2: Safe Real Filesystem Execution & Independent Verification
    // -------------------------------------------------------------
    std::string test_file_path = "c:/Users/youri/OneDrive/Desktop/vani mark 2/temp_phase7b_real_file.txt";
    std::string test_content = "VANI Mark 2 Phase 7B Real OS Validation Active";

    std::cout << "--> Executing Real Windows Filesystem Write: " << test_file_path << "\n";
    vani::capabilities::system::ToolExecutionPipelineContext ctx_fs;
    ctx_fs.capability_id = "filesystem.write";
    ctx_fs.tool_id = "filesystem.write";
    ctx_fs.task_id = "task_real_fs";
    ctx_fs.actor_id = "agent.planner";
    ctx_fs.arguments_json = "{\"path\":\"" + test_file_path + "\",\"content\":\"" + test_content + "\"}";

    auto fs_res = gateway->execute(ctx_fs);
    assert(fs_res.is_ok());

    // Independent Verification of File on Physical Disk
    AgentStep step_fs;
    step_fs.step_id = "step_fs_write";
    step_fs.capability_id = "filesystem.write";
    step_fs.arguments["path"] = test_file_path;
    step_fs.expected_postcondition = "file.exists:" + test_file_path;

    auto v_fs = verifier.verify(step_fs, fs_res.value());
    assert(v_fs.verified);
    assert(std::filesystem::exists(test_file_path));
    assert(std::filesystem::file_size(test_file_path) > 0);
    std::cout << "    [VERIFIED] " << v_fs.evidence << "\n";

    // Clean up temporary test file
    std::error_code ec;
    std::filesystem::remove(test_file_path, ec);
    assert(!std::filesystem::exists(test_file_path));
    std::cout << "    [CLEANUP] Temporary test file safely removed.\n";

    std::cout << "[PASS] REAL_WINDOWS_EXECUTION = TRUE\n";
    return 0;
    } catch (const std::exception& e) {
        std::cerr << "CAUGHT EXCEPTION: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "CAUGHT UNKNOWN EXCEPTION\n";
        return 1;
    }
}
