#pragma once

#include "../../capabilities/system/gateway/tool_gateway.hpp"
#include "../../adapters/system/windows/windows_system_adapter.hpp"
#include "../../runtime/policy/policy_engine.hpp"
#include "../../runtime/permissions/permission_service.hpp"
#include "../../runtime/event_bus/event_bus.hpp"
#include "../../runtime/audit/audit_service.hpp"
#include "../../capabilities/system/applications/application_manager.hpp"
#include "../../capabilities/system/processes/process_manager.hpp"
#include "../../capabilities/system/filesystem/filesystem_manager.hpp"
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

namespace vani::tests {

inline capabilities::system::ToolGatewayPtr create_real_windows_gateway(
    runtime::PolicyEnginePtr policy = nullptr,
    runtime::PermissionServicePtr permissions = nullptr
) {
    auto adapter = std::make_shared<adapters::system::WindowsSystemAdapter>();
    if (!policy) policy = std::make_shared<runtime::PolicyEngine>();
    if (!permissions) {
        permissions = std::make_shared<runtime::PermissionService>();
        permissions->grant_permission("agent.planner", "application.launch");
        permissions->grant_permission("agent.planner", "application.close");
        permissions->grant_permission("agent.planner", "system.status");
        permissions->grant_permission("agent.planner", "filesystem.write");
        permissions->grant_permission("agent.planner", "filesystem.read");
        permissions->grant_permission("agent.planner", "browser.open_url");
        permissions->grant_permission("user", "application.launch");
        permissions->grant_permission("user", "application.close");
        permissions->grant_permission("user", "system.status");
        permissions->grant_permission("user", "filesystem.write");
        permissions->grant_permission("user", "filesystem.read");
        permissions->grant_permission("user", "browser.open_url");
    }
    auto event_bus = std::make_shared<runtime::EventBus>();
    auto audit = std::make_shared<runtime::AuditService>();

    auto app_reg = std::make_shared<capabilities::system::ApplicationRegistry>();
    auto app_mgr = std::make_shared<capabilities::system::ApplicationManager>(adapter, app_reg);
    auto proc_mgr = std::make_shared<capabilities::system::ProcessManager>(adapter);
    auto tx_mgr = std::make_shared<capabilities::system::FileTransactionManager>();
    auto watcher = std::make_shared<capabilities::system::FileWatcher>();
    auto fs_mgr = std::make_shared<capabilities::system::FilesystemManager>(adapter, tx_mgr, watcher);
    auto term_exec = std::make_shared<capabilities::system::TerminalExecutor>(adapter);
    auto proj_mgr = std::make_shared<capabilities::system::ProjectContextManager>();
    auto browser_mgr = std::make_shared<capabilities::system::BrowserManager>();
    auto win_mgr = std::make_shared<capabilities::system::WindowManager>(adapter);
    auto input_mgr = std::make_shared<capabilities::system::InputManager>(adapter);
    auto clip_mgr = std::make_shared<capabilities::system::ClipboardManager>(adapter);
    auto screen_mgr = std::make_shared<capabilities::system::ScreenCaptureManager>(adapter);
    auto disp_mgr = std::make_shared<capabilities::system::DisplayManager>(adapter);
    auto media_mgr = std::make_shared<capabilities::system::MediaManager>(adapter);
    auto state_prov = std::make_shared<capabilities::system::SystemStateProvider>(adapter);
    auto net_mgr = std::make_shared<capabilities::system::NetworkManager>(adapter);
    auto pwr_mgr = std::make_shared<capabilities::system::PowerManager>(adapter);
    auto notif_mgr = std::make_shared<capabilities::system::NotificationManager>(adapter);
    auto journal = std::make_shared<capabilities::system::SystemActionJournal>();

    return std::make_shared<capabilities::system::ToolGateway>(
        policy, permissions, event_bus, audit,
        app_mgr, proc_mgr, fs_mgr, term_exec, proj_mgr,
        browser_mgr, win_mgr, input_mgr, clip_mgr, screen_mgr,
        disp_mgr, media_mgr, state_prov, net_mgr, pwr_mgr, notif_mgr, journal
    );
}

} // namespace vani::tests
