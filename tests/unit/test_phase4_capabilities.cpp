#include "../../capabilities/system/applications/application_registry.hpp"
#include "../../capabilities/system/applications/application_manager.hpp"
#include "../../capabilities/system/processes/process_manager.hpp"
#include "../../capabilities/system/filesystem/path_security.hpp"
#include "../../capabilities/system/filesystem/file_transaction.hpp"
#include "../../capabilities/system/filesystem/file_watcher.hpp"
#include "../../capabilities/system/filesystem/filesystem_manager.hpp"
#include "../../capabilities/system/terminal/command_policy_evaluator.hpp"
#include "../../capabilities/system/terminal/working_directory_resolver.hpp"
#include "../../capabilities/system/terminal/terminal_executor.hpp"
#include "../../capabilities/system/projects/project_context.hpp"
#include "../../capabilities/system/browser/browser_manager.hpp"
#include "../../capabilities/system/windows/window_manager.hpp"
#include "../../capabilities/system/input/input_manager.hpp"
#include "../../capabilities/system/clipboard/clipboard_manager.hpp"
#include "../../capabilities/system/screen/screen_capture_manager.hpp"
#include "../../capabilities/system/display/display_manager.hpp"
#include "../../capabilities/system/media/media_manager.hpp"
#include "../../capabilities/system/system_state/system_state_provider.hpp"
#include "../../capabilities/system/network/network_manager.hpp"
#include "../../capabilities/system/power/power_manager.hpp"
#include "../../capabilities/system/notifications/notification_manager.hpp"
#include "../../capabilities/system/journal/system_action_journal.hpp"
#include "../../adapters/system/mock/mock_system_adapter.hpp"
#include <iostream>
#include <cassert>

using namespace vani::capabilities::system;
using namespace vani::adapters::system;
using namespace vani::contracts;

void test_application_registry() {
    std::cout << "[TEST] ApplicationRegistry Deterministic Aliases...\n";
    auto registry = std::make_shared<ApplicationRegistry>();

    assert(registry->count() >= 3);
    auto chrome_opt = registry->resolve("Chrome");
    assert(chrome_opt.has_value());
    assert(chrome_opt->application_id == "com.google.chrome");

    auto vscode_opt = registry->resolve("vs code");
    assert(vscode_opt.has_value());
    assert(vscode_opt->application_id == "com.microsoft.vscode");

    auto code_opt = registry->resolve("code");
    assert(code_opt.has_value());
    assert(code_opt->application_id == "com.microsoft.vscode");

    std::cout << "  ✓ ApplicationRegistry passed\n";
}

void test_process_management() {
    std::cout << "[TEST] ProcessManager...\n";
    auto mock_adapter = std::make_shared<MockSystemAdapter>();
    auto proc_manager = std::make_shared<ProcessManager>(mock_adapter);

    auto list_res = proc_manager->list_processes();
    assert(list_res.is_success());
    assert(list_res.value().size() >= 2);

    auto start_res = proc_manager->start_process("test_worker");
    assert(start_res.is_success());
    uint32_t pid = start_res.value();

    auto inspect_res = proc_manager->inspect_process(pid);
    assert(inspect_res.is_success());
    assert(inspect_res.value().name == "test_worker");

    auto stop_res = proc_manager->stop_process(pid);
    assert(stop_res.is_success());

    std::cout << "  ✓ ProcessManager passed\n";
}

void test_path_security_and_filesystem() {
    std::cout << "[TEST] PathSecurity and FilesystemManager...\n";

    assert(PathSecurity::is_traversal_attack("../etc/passwd"));
    assert(PathSecurity::is_traversal_attack("foo/../../bar"));
    assert(!PathSecurity::is_traversal_attack("workspace/vani/main.cpp"));

    assert(PathSecurity::is_sensitive_location("/etc/shadow"));
    assert(PathSecurity::is_sensitive_location("C:/Windows/System32/config"));
    assert(PathSecurity::is_sensitive_location("/home/user/.ssh/id_rsa"));
    assert(!PathSecurity::is_sensitive_location("/workspace/vani/README.md"));

    auto mock_adapter = std::make_shared<MockSystemAdapter>();
    auto fs_manager = std::make_shared<FilesystemManager>(mock_adapter);

    auto read_res = fs_manager->read_file("/workspace/vani/README.md");
    assert(read_res.is_success());

    // Test transaction and rollback
    auto tx_mgr = fs_manager->transaction_manager();
    std::string tx_id = tx_mgr->begin_transaction("task_123");

    auto write_res = fs_manager->write_file("/workspace/vani/README.md", "Modified Content", "*", tx_id);
    assert(write_res.is_success());

    assert(fs_manager->read_file("/workspace/vani/README.md").value() == "Modified Content");

    // Rollback
    auto rollback_res = fs_manager->rollback_transaction(tx_id);
    assert(rollback_res.is_success());

    assert(fs_manager->read_file("/workspace/vani/README.md").value() == "# VANI Mark 2\nLocal-First AI Operating Layer");

    std::cout << "  ✓ PathSecurity and FilesystemManager passed\n";
}

void test_terminal_policy_and_executor() {
    std::cout << "[TEST] CommandPolicyEvaluator and TerminalExecutor...\n";

    assert(CommandPolicyEvaluator::classify_command("ls -la") == CommandRiskCategory::Safe);
    assert(CommandPolicyEvaluator::classify_command("git status") == CommandRiskCategory::Safe);
    assert(CommandPolicyEvaluator::classify_command("npm test") == CommandRiskCategory::Medium);
    assert(CommandPolicyEvaluator::classify_command("rm -rf /") == CommandRiskCategory::Critical);
    assert(CommandPolicyEvaluator::classify_command("del /f /s /q c:\\") == CommandRiskCategory::Critical);

    auto mock_adapter = std::make_shared<MockSystemAdapter>();
    auto executor = std::make_shared<TerminalExecutor>(mock_adapter);

    TerminalExecutionRequest req;
    req.command = "git status";
    auto exec_res = executor->execute(req);
    assert(exec_res.is_success());

    // Restricted mode should reject critical commands
    req.command = "rm -rf /";
    req.execution_mode = TerminalExecutionMode::Sandboxed;
    auto reject_res = executor->execute(req);
    assert(!reject_res.is_success());
    assert(reject_res.error().code == ErrorCode::SecurityViolation);

    std::cout << "  ✓ CommandPolicyEvaluator and TerminalExecutor passed\n";
}

void test_clipboard_zero_log() {
    std::cout << "[TEST] ClipboardManager Zero-Log Guarantee...\n";
    auto mock_adapter = std::make_shared<MockSystemAdapter>();
    auto clip_manager = std::make_shared<ClipboardManager>(mock_adapter);

    auto write_res = clip_manager->write_clipboard("SuperSecretPassword123!", true);
    assert(write_res.is_success());

    auto read_res = clip_manager->read_clipboard();
    assert(read_res.is_success());
    assert(read_res.value().text_content == "SuperSecretPassword123!");

    std::string sanitized = ClipboardManager::sanitize_for_audit(read_res.value());
    assert(sanitized.find("SuperSecretPassword123!") == std::string::npos);
    assert(sanitized.find("Length=23") != std::string::npos);

    std::cout << "  ✓ ClipboardManager Zero-Log passed\n";
}

void test_power_manager_confirmation() {
    std::cout << "[TEST] PowerManager Confirmation Policy...\n";
    auto mock_adapter = std::make_shared<MockSystemAdapter>();
    auto power_mgr = std::make_shared<PowerManager>(mock_adapter);

    // Unconfirmed shutdown must fail
    auto unconf_res = power_mgr->shutdown_system(false);
    assert(!unconf_res.is_success());
    assert(unconf_res.error().code == ErrorCode::PermissionDenied);

    // Confirmed shutdown succeeds
    auto conf_res = power_mgr->shutdown_system(true);
    assert(conf_res.is_success());
    assert(mock_adapter->executed_power_actions().size() == 1);
    assert(mock_adapter->executed_power_actions()[0] == PowerAction::Shutdown);

    std::cout << "  ✓ PowerManager Confirmation passed\n";
}

int main() {
    std::cout << "\n=======================================================\n";
    std::cout << "   VANI Mark 2 — Phase 4 Unit Tests Suite\n";
    std::cout << "=======================================================\n\n";

    test_application_registry();
    test_process_management();
    test_path_security_and_filesystem();
    test_terminal_policy_and_executor();
    test_clipboard_zero_log();
    test_power_manager_confirmation();

    std::cout << "\n=======================================================\n";
    std::cout << "  All Phase 4 Unit Tests Passed Successfully (100%)\n";
    std::cout << "=======================================================\n";
    return 0;
}
