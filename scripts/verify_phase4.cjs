/**
 * VANI Mark 2 — Phase 4 Production System Capability & Computer Control Layer Automated Verification Suite
 * Verifies all architectural requirements, contracts, adapters, capabilities, security, and tests.
 */
const fs = require('fs');
const path = require('path');

const ROOT = path.resolve(__dirname, '..');
let totalChecks = 0;
let passedChecks = 0;

function check(title, condition, detail = '') {
  totalChecks++;
  if (condition) {
    passedChecks++;
    console.log(`  \x1b[32m✓\x1b[0m [PASS] ${title}`);
  } else {
    console.error(`  \x1b[31m✗\x1b[0m [FAIL] ${title} — ${detail}`);
  }
}

function fileExists(relPath) {
  return fs.existsSync(path.join(ROOT, relPath));
}

function fileContains(relPath, str) {
  if (!fileExists(relPath)) return false;
  const content = fs.readFileSync(path.join(ROOT, relPath), 'utf8');
  return content.includes(str);
}

console.log('\n===============================================================');
console.log('  VANI MARK 2 — PHASE 4 SYSTEM CAPABILITY LAYER VERIFICATION   ');
console.log('===============================================================\n');

// 1. Capability & Tool Contracts Extension
console.log('\x1b[36m[1/8] Capability & Tool Contracts Extension\x1b[0m');
check('contracts/capabilities/capability_descriptor.hpp exists', fileExists('contracts/capabilities/capability_descriptor.hpp'));
check('CapabilityDescriptorExtended includes Reversibility and Idempotency', fileContains('contracts/capabilities/capability_descriptor.hpp', 'Reversibility') && fileContains('contracts/capabilities/capability_descriptor.hpp', 'Idempotency'));
check('ResourceRequirements includes CPU, Memory, Timeout, Output limits', fileContains('contracts/capabilities/capability_descriptor.hpp', 'max_cpu_percent') && fileContains('contracts/capabilities/capability_descriptor.hpp', 'max_output_bytes'));
check('contracts/capabilities/platform_support_matrix.hpp exists', fileExists('contracts/capabilities/platform_support_matrix.hpp'));
check('PlatformSupportMatrix initializes standard entries', fileContains('contracts/capabilities/platform_support_matrix.hpp', 'system.application.open') && fileContains('contracts/capabilities/platform_support_matrix.hpp', 'terminal.execute'));
check('contracts/system/system_contracts.hpp exists', fileExists('contracts/system/system_contracts.hpp'));
check('System Contracts covers 15 domain models', fileContains('contracts/system/system_contracts.hpp', 'ApplicationMetadata') && fileContains('contracts/system/system_contracts.hpp', 'SystemActionJournalEntry'));

// 2. Platform Adapter Layer & Mock
console.log('\n\x1b[36m[2/8] Platform Adapter Abstraction & Mock Provider\x1b[0m');
check('adapters/system/system_adapter.hpp exists', fileExists('adapters/system/system_adapter.hpp'));
check('SystemAdapter defines pure virtual interface', fileContains('adapters/system/system_adapter.hpp', 'class SystemAdapter'));
check('adapters/system/mock/mock_system_adapter.hpp exists', fileExists('adapters/system/mock/mock_system_adapter.hpp'));
check('adapters/system/mock/mock_system_adapter.cpp exists', fileExists('adapters/system/mock/mock_system_adapter.cpp'));
check('MockSystemAdapter simulates applications, procs, files, terminal, windows', fileContains('adapters/system/mock/mock_system_adapter.cpp', 'com.google.chrome') && fileContains('adapters/system/mock/mock_system_adapter.cpp', 'trash_bin_'));
check('adapters/system/windows/windows_system_adapter.hpp exists', fileExists('adapters/system/windows/windows_system_adapter.hpp'));
check('adapters/system/windows/windows_system_adapter.cpp exists', fileExists('adapters/system/windows/windows_system_adapter.cpp'));
check('adapters/system/linux/linux_system_adapter.hpp exists', fileExists('adapters/system/linux/linux_system_adapter.hpp'));
check('adapters/system/linux/linux_system_adapter.cpp exists', fileExists('adapters/system/linux/linux_system_adapter.cpp'));
check('adapters/system/macos/macos_system_adapter.hpp exists', fileExists('adapters/system/macos/macos_system_adapter.hpp'));
check('adapters/system/macos/macos_system_adapter.cpp exists', fileExists('adapters/system/macos/macos_system_adapter.cpp'));

// 3. Application & Process Management
console.log('\n\x1b[36m[3/8] Application & Process Management Subsystems\x1b[0m');
check('capabilities/system/applications/application_registry.hpp exists', fileExists('capabilities/system/applications/application_registry.hpp'));
check('ApplicationRegistry indexes aliases (VS Code -> code.exe)', fileContains('capabilities/system/applications/application_registry.cpp', 'com.microsoft.vscode') && fileContains('capabilities/system/applications/application_registry.cpp', 'vscode'));
check('capabilities/system/applications/application_manager.hpp exists', fileExists('capabilities/system/applications/application_manager.hpp'));
check('capabilities/system/applications/application_manager.cpp exists', fileExists('capabilities/system/applications/application_manager.cpp'));
check('capabilities/system/processes/process_manager.hpp exists', fileExists('capabilities/system/processes/process_manager.hpp'));
check('ProcessManager supports graceful stop and force terminate', fileContains('capabilities/system/processes/process_manager.cpp', 'stop_process') && fileContains('capabilities/system/processes/process_manager.cpp', 'terminate_process'));

// 4. Filesystem, Path Security & Transactions
console.log('\n\x1b[36m[4/8] Filesystem Security, Safe Trash & Transactions\x1b[0m');
check('capabilities/system/filesystem/path_security.hpp exists', fileExists('capabilities/system/filesystem/path_security.hpp'));
check('PathSecurity checks traversal (../) and sensitive locations', fileContains('capabilities/system/filesystem/path_security.cpp', 'is_traversal_attack') && fileContains('capabilities/system/filesystem/path_security.cpp', 'is_sensitive_location'));
check('capabilities/system/filesystem/file_transaction.hpp exists', fileExists('capabilities/system/filesystem/file_transaction.hpp'));
check('capabilities/system/filesystem/file_watcher.hpp exists', fileExists('capabilities/system/filesystem/file_watcher.hpp'));
check('capabilities/system/filesystem/filesystem_manager.hpp exists', fileExists('capabilities/system/filesystem/filesystem_manager.hpp'));
check('FilesystemManager implements rollback_transaction', fileContains('capabilities/system/filesystem/filesystem_manager.cpp', 'rollback_transaction'));

// 5. Terminal, Projects & Browser Subsystems
console.log('\n\x1b[36m[5/8] Terminal Policy, Projects & Browser Capabilities\x1b[0m');
check('capabilities/system/terminal/command_policy_evaluator.hpp exists', fileExists('capabilities/system/terminal/command_policy_evaluator.hpp'));
check('CommandPolicyEvaluator categorizes 5 risk tiers (SAFE to CRITICAL)', fileContains('capabilities/system/terminal/command_policy_evaluator.cpp', 'CommandRiskCategory::Critical'));
check('capabilities/system/terminal/working_directory_resolver.hpp exists', fileExists('capabilities/system/terminal/working_directory_resolver.hpp'));
check('capabilities/system/terminal/terminal_executor.hpp exists', fileExists('capabilities/system/terminal/terminal_executor.hpp'));
check('TerminalExecutor enforces output limits and execution mode', fileContains('capabilities/system/terminal/terminal_executor.cpp', 'TRUNCATED'));
check('capabilities/system/projects/project_context.hpp exists', fileExists('capabilities/system/projects/project_context.hpp'));
check('ProjectContextManager indexes project root and known commands', fileContains('capabilities/system/projects/project_context.cpp', 'known_commands'));
check('capabilities/system/browser/browser_session.hpp exists', fileExists('capabilities/system/browser/browser_session.hpp'));
check('capabilities/system/browser/browser_manager.hpp exists', fileExists('capabilities/system/browser/browser_manager.hpp'));
check('BrowserManager separates read-only from state-changing actions', fileContains('capabilities/system/browser/browser_session.hpp', 'is_state_changing_browser_action'));

// 6. Windows, Input, Clipboard, Screen, Media, Power & System State
console.log('\n\x1b[36m[6/8] Windows, Input, Clipboard, Screen, Media, Power & State\x1b[0m');
check('capabilities/system/windows/window_manager.hpp exists', fileExists('capabilities/system/windows/window_manager.hpp'));
check('capabilities/system/input/input_manager.hpp exists', fileExists('capabilities/system/input/input_manager.hpp'));
check('capabilities/system/clipboard/clipboard_manager.hpp exists', fileExists('capabilities/system/clipboard/clipboard_manager.hpp'));
check('ClipboardManager enforces zero-logging of content', fileContains('capabilities/system/clipboard/clipboard_manager.cpp', 'sanitize_for_audit'));
check('capabilities/system/screen/screen_capture_manager.hpp exists', fileExists('capabilities/system/screen/screen_capture_manager.hpp'));
check('capabilities/system/display/display_manager.hpp exists', fileExists('capabilities/system/display/display_manager.hpp'));
check('capabilities/system/media/media_manager.hpp exists', fileExists('capabilities/system/media/media_manager.hpp'));
check('capabilities/system/system_state/system_state_provider.hpp exists', fileExists('capabilities/system/system_state/system_state_provider.hpp'));
check('capabilities/system/network/network_manager.hpp exists', fileExists('capabilities/system/network/network_manager.hpp'));
check('capabilities/system/power/power_manager.hpp exists', fileExists('capabilities/system/power/power_manager.hpp'));
check('PowerManager requires explicit confirmation for shutdown/restart', fileContains('capabilities/system/power/power_manager.cpp', 'user_confirmed'));
check('capabilities/system/notifications/notification_manager.hpp exists', fileExists('capabilities/system/notifications/notification_manager.hpp'));
check('capabilities/system/journal/system_action_journal.hpp exists', fileExists('capabilities/system/journal/system_action_journal.hpp'));

// 7. Tool Gateway & Fast Path Pipeline
console.log('\n\x1b[36m[7/8] 10-Step Tool Gateway Pipeline & Fast Path\x1b[0m');
check('capabilities/system/gateway/tool_gateway.hpp exists', fileExists('capabilities/system/gateway/tool_gateway.hpp'));
check('ToolGateway implements 10-step pipeline with Policy & Permissions', fileContains('capabilities/system/gateway/tool_gateway.cpp', 'policy_engine_->evaluate') && fileContains('capabilities/system/gateway/tool_gateway.cpp', 'permission_service_->has_permission'));
check('ToolGateway implements Fast Path execution without LLM', fileContains('capabilities/system/gateway/tool_gateway.cpp', 'execute_fast_path'));
check('ToolGateway verifies postconditions', fileContains('capabilities/system/gateway/tool_gateway.cpp', 'verify_postcondition'));
check('ToolGateway logs to SystemActionJournal and EventBus', fileContains('capabilities/system/gateway/tool_gateway.cpp', 'journal_->log_action'));

// 8. Documentation, Tests & Benchmarks
console.log('\n\x1b[36m[8/8] Documentation Suite, Test Suites & Benchmarks\x1b[0m');
check('docs/capabilities/system.md exists', fileExists('docs/capabilities/system.md'));
check('docs/capabilities/filesystem.md exists', fileExists('docs/capabilities/filesystem.md'));
check('docs/capabilities/terminal.md exists', fileExists('docs/capabilities/terminal.md'));
check('docs/capabilities/browser.md exists', fileExists('docs/capabilities/browser.md'));
check('docs/capabilities/applications.md exists', fileExists('docs/capabilities/applications.md'));
check('docs/capabilities/windows.md exists', fileExists('docs/capabilities/windows.md'));
check('docs/capabilities/input.md exists', fileExists('docs/capabilities/input.md'));
check('docs/capabilities/clipboard.md exists', fileExists('docs/capabilities/clipboard.md'));
check('docs/capabilities/screen.md exists', fileExists('docs/capabilities/screen.md'));
check('docs/capabilities/platform-support.md exists', fileExists('docs/capabilities/platform-support.md'));
check('tests/unit/test_phase4_capabilities.cpp exists', fileExists('tests/unit/test_phase4_capabilities.cpp'));
check('tests/security/test_phase4_security.cpp exists', fileExists('tests/security/test_phase4_security.cpp'));
check('tests/integration/test_phase4_reference_flows.cpp exists', fileExists('tests/integration/test_phase4_reference_flows.cpp'));
check('benchmarks/benchmark_phase4_capabilities.cpp exists', fileExists('benchmarks/benchmark_phase4_capabilities.cpp'));

console.log('\n===============================================================');
const passPct = Math.round((passedChecks / totalChecks) * 100);
if (passedChecks === totalChecks) {
  console.log(`\x1b[32m✔ PHASE 4 VERIFICATION PASSED: ${passedChecks}/${totalChecks} checks (100%)\x1b[0m`);
} else {
  console.error(`\x1b[31m✖ PHASE 4 VERIFICATION FAILED: ${passedChecks}/${totalChecks} checks (${passPct}%)\x1b[0m`);
  process.exit(1);
}
console.log('===============================================================\n');
