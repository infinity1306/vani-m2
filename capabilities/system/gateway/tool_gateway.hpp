#pragma once

#include "../../../contracts/tools/tool.hpp"
#include "../../../contracts/capabilities/capability_descriptor.hpp"
#include "../../../contracts/capabilities/platform_support_matrix.hpp"
#include "../../../contracts/system/system_contracts.hpp"
#include "../../../contracts/common/result.hpp"
#include "../../../runtime/policy/policy_engine.hpp"
#include "../../../runtime/permissions/permission_service.hpp"
#include "../../../runtime/event_bus/event_bus.hpp"
#include "../../../runtime/audit/audit_service.hpp"
#include "../applications/application_manager.hpp"
#include "../processes/process_manager.hpp"
#include "../filesystem/filesystem_manager.hpp"
#include "../terminal/terminal_executor.hpp"
#include "../projects/project_context.hpp"
#include "../browser/browser_manager.hpp"
#include "../windows/window_manager.hpp"
#include "../input/input_manager.hpp"
#include "../clipboard/clipboard_manager.hpp"
#include "../screen/screen_capture_manager.hpp"
#include "../display/display_manager.hpp"
#include "../media/media_manager.hpp"
#include "../system_state/system_state_provider.hpp"
#include "../network/network_manager.hpp"
#include "../power/power_manager.hpp"
#include "../notifications/notification_manager.hpp"
#include "../journal/system_action_journal.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include "../undo/undo_manager.hpp"
#include "../../../contracts/memory/memory_provider.hpp"
#include "../../../runtime/scheduler/scheduler.hpp"

namespace vani::capabilities::system {

struct ToolExecutionPipelineContext {
    std::string capability_id;
    std::string tool_id;
    std::string actor_id{"user"};
    std::string task_id;
    std::string session_id;
    std::string correlation_id;
    std::string arguments_json;
    std::unordered_map<std::string, std::string> execution_context;
    bool user_confirmed{false};
    contracts::CancellationToken cancellation_token{contracts::CancellationToken::none()};
};

class ToolGateway {
public:
    ToolGateway(
        runtime::PolicyEnginePtr policy_engine,
        runtime::PermissionServicePtr permission_service,
        runtime::EventBusPtr event_bus,
        runtime::AuditServicePtr audit_service,
        ApplicationManagerPtr app_manager,
        ProcessManagerPtr proc_manager,
        FilesystemManagerPtr fs_manager,
        TerminalExecutorPtr term_executor,
        ProjectContextManagerPtr proj_manager,
        BrowserManagerPtr browser_manager,
        WindowManagerPtr win_manager,
        InputManagerPtr input_manager,
        ClipboardManagerPtr clipboard_manager,
        ScreenCaptureManagerPtr screen_manager,
        DisplayManagerPtr display_manager,
        MediaManagerPtr media_manager,
        SystemStateProviderPtr state_provider,
        NetworkManagerPtr network_manager,
        PowerManagerPtr power_manager,
        NotificationManagerPtr notif_manager,
        SystemActionJournalPtr journal
    );
    ~ToolGateway() = default;

    // 10-Step Pipeline Execution Gateway
    contracts::Result<contracts::ToolResult> execute(const ToolExecutionPipelineContext& context);

    // Fast Path for deterministic local commands (sub-millisecond without LLM)
    contracts::Result<contracts::ToolResult> execute_fast_path(
        const std::string& intent_name,
        const std::unordered_map<std::string, std::string>& parameters,
        const std::string& actor_id = "user.fast_path"
    );

    // Subsystem accessors
    [[nodiscard]] ApplicationManagerPtr application_manager() const noexcept { return app_manager_; }
    [[nodiscard]] ProcessManagerPtr process_manager() const noexcept { return proc_manager_; }
    [[nodiscard]] FilesystemManagerPtr filesystem_manager() const noexcept { return fs_manager_; }
    [[nodiscard]] TerminalExecutorPtr terminal_executor() const noexcept { return term_executor_; }
    [[nodiscard]] ProjectContextManagerPtr project_manager() const noexcept { return proj_manager_; }
    [[nodiscard]] BrowserManagerPtr browser_manager() const noexcept { return browser_manager_; }
    [[nodiscard]] WindowManagerPtr window_manager() const noexcept { return win_manager_; }
    [[nodiscard]] InputManagerPtr input_manager() const noexcept { return input_manager_; }
    [[nodiscard]] ClipboardManagerPtr clipboard_manager() const noexcept { return clipboard_manager_; }
    [[nodiscard]] ScreenCaptureManagerPtr screen_manager() const noexcept { return screen_manager_; }
    [[nodiscard]] DisplayManagerPtr display_manager() const noexcept { return display_manager_; }
    [[nodiscard]] MediaManagerPtr media_manager() const noexcept { return media_manager_; }
    [[nodiscard]] SystemStateProviderPtr state_provider() const noexcept { return state_provider_; }
    [[nodiscard]] NetworkManagerPtr network_manager() const noexcept { return network_manager_; }
    [[nodiscard]] PowerManagerPtr power_manager() const noexcept { return power_manager_; }
    [[nodiscard]] NotificationManagerPtr notification_manager() const noexcept { return notif_manager_; }
    [[nodiscard]] SystemActionJournalPtr action_journal() const noexcept { return journal_; }
    [[nodiscard]] UndoManagerPtr undo_manager() const noexcept { return undo_manager_; }
    [[nodiscard]] contracts::MemoryProviderPtr memory_provider() const noexcept { return memory_provider_; }
    [[nodiscard]] runtime::SchedulerPtr scheduler() const noexcept { return scheduler_; }

    void set_undo_manager(UndoManagerPtr undo_mgr) noexcept { undo_manager_ = undo_mgr; }
    void set_memory_provider(contracts::MemoryProviderPtr mem_prov) noexcept { memory_provider_ = mem_prov; }
    void set_scheduler(runtime::SchedulerPtr sched) noexcept { scheduler_ = sched; }

private:
    contracts::Result<std::string> dispatch_capability(
        const std::string& capability_id,
        const std::string& arguments_json,
        const ToolExecutionPipelineContext& context
    );

    contracts::Result<void> verify_postcondition(
        const std::string& capability_id,
        const std::string& result_output
    );

    runtime::PolicyEnginePtr policy_engine_;
    runtime::PermissionServicePtr permission_service_;
    runtime::EventBusPtr event_bus_;
    runtime::AuditServicePtr audit_service_;

    ApplicationManagerPtr app_manager_;
    ProcessManagerPtr proc_manager_;
    FilesystemManagerPtr fs_manager_;
    TerminalExecutorPtr term_executor_;
    ProjectContextManagerPtr proj_manager_;
    BrowserManagerPtr browser_manager_;
    WindowManagerPtr win_manager_;
    InputManagerPtr input_manager_;
    ClipboardManagerPtr clipboard_manager_;
    ScreenCaptureManagerPtr screen_manager_;
    DisplayManagerPtr display_manager_;
    MediaManagerPtr media_manager_;
    SystemStateProviderPtr state_provider_;
    NetworkManagerPtr network_manager_;
    PowerManagerPtr power_manager_;
    NotificationManagerPtr notif_manager_;
    SystemActionJournalPtr journal_;
    UndoManagerPtr undo_manager_{nullptr};
    contracts::MemoryProviderPtr memory_provider_{nullptr};
    runtime::SchedulerPtr scheduler_{nullptr};
};

using ToolGatewayPtr = std::shared_ptr<ToolGateway>;

} // namespace vani::capabilities::system
