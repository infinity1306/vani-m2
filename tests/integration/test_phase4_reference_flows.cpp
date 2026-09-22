#include "../../capabilities/system/gateway/tool_gateway.hpp"
#include "../../adapters/system/mock/mock_system_adapter.hpp"
#include "../../runtime/policy/policy_engine.hpp"
#include "../../runtime/permissions/permission_service.hpp"
#include "../../runtime/event_bus/event_bus.hpp"
#include <iostream>
#include <cassert>

using namespace vani::capabilities::system;
using namespace vani::adapters::system;
using namespace vani::contracts;
using namespace vani::runtime;

std::shared_ptr<ToolGateway> create_test_gateway(std::shared_ptr<MockSystemAdapter> mock_adapter) {
    auto policy = std::make_shared<PolicyEngine>();
    auto perm = std::make_shared<PermissionService>();
    auto bus = std::make_shared<EventBus>();
    auto audit = std::make_shared<AuditService>();

    auto app_reg = std::make_shared<ApplicationRegistry>();
    auto app_mgr = std::make_shared<ApplicationManager>(mock_adapter, app_reg);
    auto proc_mgr = std::make_shared<ProcessManager>(mock_adapter);
    auto tx_mgr = std::make_shared<FileTransactionManager>();
    auto watcher = std::make_shared<FileWatcher>();
    auto fs_mgr = std::make_shared<FilesystemManager>(mock_adapter, tx_mgr, watcher);
    auto term_exec = std::make_shared<TerminalExecutor>(mock_adapter);
    auto proj_mgr = std::make_shared<ProjectContextManager>();
    auto browser_mgr = std::make_shared<BrowserManager>();
    auto win_mgr = std::make_shared<WindowManager>(mock_adapter);
    auto input_mgr = std::make_shared<InputManager>(mock_adapter);
    auto clip_mgr = std::make_shared<ClipboardManager>(mock_adapter);
    auto screen_mgr = std::make_shared<ScreenCaptureManager>(mock_adapter);
    auto display_mgr = std::make_shared<DisplayManager>(mock_adapter);
    auto media_mgr = std::make_shared<MediaManager>(mock_adapter);
    auto state_prov = std::make_shared<SystemStateProvider>(mock_adapter);
    auto net_mgr = std::make_shared<NetworkManager>(mock_adapter);
    auto power_mgr = std::make_shared<PowerManager>(mock_adapter);
    auto notif_mgr = std::make_shared<NotificationManager>(mock_adapter);
    auto journal = std::make_shared<SystemActionJournal>();

    return std::make_shared<ToolGateway>(
        policy, perm, bus, audit,
        app_mgr, proc_mgr, fs_mgr, term_exec, proj_mgr,
        browser_mgr, win_mgr, input_mgr, clip_mgr, screen_mgr,
        display_mgr, media_mgr, state_prov, net_mgr, power_mgr,
        notif_mgr, journal
    );
}

void test_reference_flow_open_application() {
    std::cout << "[FLOW 1] 'VANI, Chrome kholo' (Open Application)...\n";
    auto mock_adapter = std::make_shared<MockSystemAdapter>();
    auto gateway = create_test_gateway(mock_adapter);

    ToolExecutionPipelineContext ctx;
    ctx.capability_id = "system.application.open";
    ctx.arguments_json = "chrome";
    ctx.task_id = "task_open_app";

    auto res = gateway->execute(ctx);
    assert(res.is_success());
    assert(res.value().success);
    assert(res.value().output_json.find("com.google.chrome") != std::string::npos);

    // Verify postcondition: app is marked running in adapter
    auto running = mock_adapter->list_running_applications();
    assert(running.is_success());
    assert(!running.value().empty());
    assert(running.value()[0].application_id == "com.google.chrome");

    std::cout << "  ✓ Reference Flow 1: Open Application Passed\n";
}

void test_reference_flow_run_project() {
    std::cout << "[FLOW 2] 'VANI, mera React project run kar' (Run Project)...\n";
    auto mock_adapter = std::make_shared<MockSystemAdapter>();
    auto gateway = create_test_gateway(mock_adapter);

    // 1. Resolve project
    auto proj_opt = gateway->project_manager()->get_project("proj_react_frontend");
    assert(proj_opt.has_value());
    const auto& proj = *proj_opt;

    // 2. Resolve command & cwd
    auto cmd_it = proj.known_commands.find("dev");
    assert(cmd_it != proj.known_commands.end());
    std::string command = cmd_it->second; // "npm run dev"

    ToolExecutionPipelineContext ctx;
    ctx.capability_id = "terminal.execute";
    ctx.arguments_json = command;
    ctx.task_id = "task_run_project";

    auto res = gateway->execute(ctx);
    assert(res.is_success());
    assert(res.value().success);

    std::cout << "  ✓ Reference Flow 2: Run Project Passed\n";
}

void test_reference_flow_delete_and_rollback() {
    std::cout << "[FLOW 3] 'Ye file delete kar' & 'Undo' (Safe Trash & Rollback)...\n";
    auto mock_adapter = std::make_shared<MockSystemAdapter>();
    auto gateway = create_test_gateway(mock_adapter);

    auto tx_mgr = gateway->filesystem_manager()->transaction_manager();
    std::string tx_id = tx_mgr->begin_transaction("task_delete_flow");

    // Write a test file first
    auto write_res = gateway->filesystem_manager()->write_file("/workspace/temp.txt", "Delete me later", "*", tx_id);
    assert(write_res.is_success());

    // Delete file with safe trash
    auto del_res = gateway->filesystem_manager()->delete_file("/workspace/temp.txt", true, "*", tx_id);
    assert(del_res.is_success());

    // Check safe trash staging
    assert(mock_adapter->trash_bin().size() == 1);
    assert(mock_adapter->trash_bin()[0] == "/workspace/temp.txt");

    // Rollback / Undo
    auto undo_res = gateway->filesystem_manager()->rollback_transaction(tx_id);
    assert(undo_res.is_success());

    std::cout << "  ✓ Reference Flow 3: Safe Trash & Rollback Passed\n";
}

void test_reference_flow_browser() {
    std::cout << "[FLOW 4] 'Chrome mein GitHub kholo' (Browser Navigation)...\n";
    auto mock_adapter = std::make_shared<MockSystemAdapter>();
    auto gateway = create_test_gateway(mock_adapter);

    ToolExecutionPipelineContext ctx;
    ctx.capability_id = "browser.navigate";
    ctx.arguments_json = "https://github.com";
    ctx.task_id = "task_browser_flow";

    auto res = gateway->execute(ctx);
    assert(res.is_success());
    assert(res.value().success);

    std::cout << "  ✓ Reference Flow 4: Browser Navigation Passed\n";
}

void test_reference_flow_fast_path() {
    std::cout << "[FLOW 5] Fast Path Deterministic Dispatch (< 1 ms, No LLM)...\n";
    auto mock_adapter = std::make_shared<MockSystemAdapter>();
    auto gateway = create_test_gateway(mock_adapter);

    // Fast Path 1: Set volume 65
    auto vol_res = gateway->execute_fast_path("set_volume", {{"level", "65"}});
    assert(vol_res.is_success());
    assert(mock_adapter->get_media_status().value().volume_percent == 65);

    // Fast Path 2: Get battery
    auto batt_res = gateway->execute_fast_path("get_system_state", {});
    assert(batt_res.is_success());
    assert(batt_res.value().output_json.find("battery") != std::string::npos);

    // Fast Path 3: Take screenshot
    auto screen_res = gateway->execute_fast_path("take_screenshot", {});
    assert(screen_res.is_success());
    assert(screen_res.value().output_json.find("capture_id") != std::string::npos);

    std::cout << "  ✓ Reference Flow 5: Fast Path Execution Passed\n";
}

int main() {
    std::cout << "\n=======================================================\n";
    std::cout << "   VANI Mark 2 — Phase 4 Reference Flows Suite\n";
    std::cout << "=======================================================\n\n";

    test_reference_flow_open_application();
    test_reference_flow_run_project();
    test_reference_flow_delete_and_rollback();
    test_reference_flow_browser();
    test_reference_flow_fast_path();

    std::cout << "\n=======================================================\n";
    std::cout << "  All Phase 4 Reference Flows Succeeded (100%)\n";
    std::cout << "=======================================================\n";
    return 0;
}
