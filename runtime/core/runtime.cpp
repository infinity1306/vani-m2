#include "runtime.hpp"
#include "../../storage/in_memory_task_repository.hpp"
#include "../../storage/in_memory_session_repository.hpp"
#include "../../storage/in_memory_audit_repository.hpp"
#include "../../storage/in_memory_scheduler_repository.hpp"
#include "../../storage/file_scheduler_repository.hpp"
#include "../../capabilities/system/gateway/tool_gateway.hpp"
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
#include "../../capabilities/system/undo/undo_manager.hpp"
#include "../../adapters/memory/persistent_memory_provider.hpp"

#if defined(_WIN32)
#include "../../adapters/system/windows/windows_system_adapter.hpp"
#elif defined(__APPLE__)
#include "../../adapters/system/macos/macos_system_adapter.hpp"
#else
#include "../../adapters/system/linux/linux_system_adapter.hpp"
#endif

#include <iostream>

namespace vani::runtime {

VaniRuntime::VaniRuntime(const config::RuntimeProfile& profile) {
    build_default_context(profile);
}

VaniRuntime::VaniRuntime(RuntimeContextPtr custom_context)
    : context_(std::move(custom_context)) {}

VaniRuntime::~VaniRuntime() {
    shutdown();
}

void VaniRuntime::build_default_context(const config::RuntimeProfile& profile) {
    context_ = std::make_shared<RuntimeContext>();

    context_->config_manager = std::make_shared<config::ConfigManager>(profile);
    context_->health_service = std::make_shared<observability::HealthService>();
    context_->lifecycle_manager = std::make_shared<LifecycleManager>();

    // Repositories
    context_->task_repository = std::make_shared<storage::InMemoryTaskRepository>();
    context_->session_repository = std::make_shared<storage::InMemorySessionRepository>();
    context_->audit_repository = std::make_shared<storage::InMemoryAuditRepository>();
    context_->scheduler_repository = std::make_shared<storage::FileSchedulerRepository>();

    // Event Bus
    context_->event_bus = std::make_shared<EventBus>(2, 2000);

    // Audit Service
    context_->audit_service = std::make_shared<AuditService>(
        context_->event_bus,
        context_->audit_repository
    );

    // Policy & Permissions
    context_->policy_engine = std::make_shared<PolicyEngine>();
    context_->permission_service = std::make_shared<PermissionService>();

    // Capability Registry
    context_->capability_registry = std::make_shared<CapabilityRegistry>();

    // Task Manager
    auto recovery = std::make_shared<RecoveryManager>();
    context_->task_manager = std::make_shared<TaskManager>(
        context_->event_bus,
        context_->task_repository,
        recovery
    );

    // Session & Context
    context_->session_manager = std::make_shared<SessionManager>(context_->session_repository);
    context_->context_manager = std::make_shared<ContextManager>();

    // Scheduler
    context_->scheduler = std::make_shared<Scheduler>(
        context_->task_manager,
        context_->scheduler_repository
    );

    // Resource Manager
    context_->resource_manager = std::make_shared<ResourceManager>();

    // Watchdog
    context_->watchdog = std::make_shared<Watchdog>(
        context_->task_manager,
        context_->health_service,
        context_->event_bus
    );

    // Platform-specific System Adapter
#if defined(_WIN32)
    auto system_adapter = std::make_shared<adapters::system::WindowsSystemAdapter>();
#elif defined(__APPLE__)
    auto system_adapter = std::make_shared<adapters::system::MacOSSystemAdapter>();
#else
    auto system_adapter = std::make_shared<adapters::system::LinuxSystemAdapter>();
#endif

    // Capabilities and Managers
    auto app_reg = std::make_shared<capabilities::system::ApplicationRegistry>();
    auto app_mgr = std::make_shared<capabilities::system::ApplicationManager>(system_adapter, app_reg);
    auto proc_mgr = std::make_shared<capabilities::system::ProcessManager>(system_adapter);
    auto tx_mgr = std::make_shared<capabilities::system::FileTransactionManager>();
    auto watcher = std::make_shared<capabilities::system::FileWatcher>();
    auto fs_mgr = std::make_shared<capabilities::system::FilesystemManager>(system_adapter, tx_mgr, watcher);
    auto term_exec = std::make_shared<capabilities::system::TerminalExecutor>(system_adapter);
    auto proj_mgr = std::make_shared<capabilities::system::ProjectContextManager>();
    auto browser_mgr = std::make_shared<capabilities::system::BrowserManager>();
    auto win_mgr = std::make_shared<capabilities::system::WindowManager>(system_adapter);
    auto input_mgr = std::make_shared<capabilities::system::InputManager>(system_adapter);
    auto clip_mgr = std::make_shared<capabilities::system::ClipboardManager>(system_adapter);
    auto screen_mgr = std::make_shared<capabilities::system::ScreenCaptureManager>(system_adapter);
    auto disp_mgr = std::make_shared<capabilities::system::DisplayManager>(system_adapter);
    auto media_mgr = std::make_shared<capabilities::system::MediaManager>(system_adapter);
    auto state_prov = std::make_shared<capabilities::system::SystemStateProvider>(system_adapter);
    auto net_mgr = std::make_shared<capabilities::system::NetworkManager>(system_adapter);
    auto pwr_mgr = std::make_shared<capabilities::system::PowerManager>(system_adapter);
    auto notif_mgr = std::make_shared<capabilities::system::NotificationManager>(system_adapter);
    auto journal = std::make_shared<capabilities::system::SystemActionJournal>();

    auto undo_mgr = std::make_shared<capabilities::system::UndoManager>(system_adapter);
    auto mem_prov = std::make_shared<adapters::memory::PersistentMemoryProvider>();

    context_->tool_gateway = std::make_shared<capabilities::system::ToolGateway>(
        context_->policy_engine,
        context_->permission_service,
        context_->event_bus,
        context_->audit_service,
        app_mgr, proc_mgr, fs_mgr, term_exec, proj_mgr,
        browser_mgr, win_mgr, input_mgr, clip_mgr, screen_mgr,
        disp_mgr, media_mgr, state_prov, net_mgr, pwr_mgr, notif_mgr, journal
    );

    context_->tool_gateway->set_undo_manager(undo_mgr);
    context_->tool_gateway->set_memory_provider(mem_prov);
    context_->tool_gateway->set_scheduler(context_->scheduler);

    // Grant standard base permissions to user and agent
    const std::vector<std::string> base_permissions = {
        "application.launch", "application.close", "system.application.list",
        "system.process.list", "filesystem.read", "filesystem.write",
        "terminal.execute", "browser.open", "browser.navigate", "browser.extract_text",
        "window.list", "clipboard.read", "clipboard.write", "screen.capture",
        "display.list", "media.play", "media.pause", "media.volume", "media.youtube.play",
        "system.status", "system.notification.send", "undo.execute", "undo.list",
        "memory.store", "memory.search", "memory.recall", "memory.forget",
        "weather.get", "web.search", "reminder.create", "reminder.list", "reminder.cancel"
    };

    for (const auto& perm : base_permissions) {
        context_->permission_service->grant_permission("user", perm);
        context_->permission_service->grant_permission("agent.planner", perm);
        context_->permission_service->grant_permission("user.fast_path", perm);
    }
}

contracts::Result<void> VaniRuntime::initialize() {
    auto trans_res = context_->lifecycle_manager->transition_to(LifecycleState::Initializing);
    if (trans_res.is_err()) {
        return trans_res;
    }

    observability::Logger::instance().info("runtime", "Initializing VANI Runtime Subsystems...");

    // Register all core capabilities in CapabilityRegistry
    auto register_cap = [this](const std::string& id, const std::string& desc,
                               contracts::RiskLevel risk, CapabilityType type = CapabilityType::Tool,
                               const std::vector<std::string>& perms = {}) {
        CapabilityDescriptor cd;
        cd.id = id;
        cd.description = desc;
        cd.risk_level = risk;
        cd.type = type;
        cd.provider_id = "vani.core";
        cd.availability = CapabilityAvailability::Available;
        cd.required_permissions = perms.empty() ? std::vector<std::string>{id} : perms;
        context_->capability_registry->register_capability(cd);
    };

    register_cap("application.launch", "Launch desktop applications", contracts::RiskLevel::Low);
    register_cap("application.close", "Close running desktop application", contracts::RiskLevel::Medium);
    register_cap("system.application.list", "List installed and running applications", contracts::RiskLevel::Low);
    register_cap("system.process.list", "Inspect live OS process table", contracts::RiskLevel::Low);
    register_cap("system.process.stop", "Terminate process by PID", contracts::RiskLevel::High);
    register_cap("filesystem.read", "Read local file contents within scope", contracts::RiskLevel::Low);
    register_cap("filesystem.write", "Write or create local files with undo support", contracts::RiskLevel::Medium);
    register_cap("filesystem.delete", "Delete files safely to trash", contracts::RiskLevel::High);
    register_cap("terminal.execute", "Execute shell/terminal command", contracts::RiskLevel::High);
    register_cap("browser.open", "Open URL in default web browser", contracts::RiskLevel::Low);
    register_cap("browser.navigate", "Navigate active browser session", contracts::RiskLevel::Low);
    register_cap("browser.extract_text", "Extract clean readable text from live web page", contracts::RiskLevel::Low);
    register_cap("window.list", "List visible desktop application windows", contracts::RiskLevel::Low);
    register_cap("clipboard.read", "Read current text from OS clipboard", contracts::RiskLevel::Low);
    register_cap("clipboard.write", "Write text to OS clipboard", contracts::RiskLevel::Low);
    register_cap("screen.capture", "Capture live desktop screenshot", contracts::RiskLevel::Low);
    register_cap("display.list", "List connected display monitors and resolutions", contracts::RiskLevel::Low);
    register_cap("media.play", "Send media play signal", contracts::RiskLevel::Low);
    register_cap("media.pause", "Send media pause signal", contracts::RiskLevel::Low);
    register_cap("media.volume", "Adjust system audio volume", contracts::RiskLevel::Low);
    register_cap("media.youtube.play", "Search and play YouTube video on command", contracts::RiskLevel::Low);
    register_cap("system.status", "Get live OS state, CPU, and battery status", contracts::RiskLevel::Low);
    register_cap("system.lock", "Lock current desktop session workstation", contracts::RiskLevel::Medium);
    register_cap("system.shutdown", "Initiate system power shutdown with confirmation", contracts::RiskLevel::Critical);
    register_cap("system.notification.send", "Display native OS desktop notification toast", contracts::RiskLevel::Low);
    register_cap("undo.execute", "Reversibly rollback previous file/clipboard mutation", contracts::RiskLevel::Medium);
    register_cap("undo.list", "List recent undoable actions in session", contracts::RiskLevel::Low);
    register_cap("memory.store", "Store memory item in persistent disk store", contracts::RiskLevel::Low);
    register_cap("memory.search", "Search persistent semantic and episodic memory", contracts::RiskLevel::Low);
    register_cap("memory.recall", "Recall memory items by keyword query", contracts::RiskLevel::Low);
    register_cap("memory.forget", "Forget persistent memory item by ID", contracts::RiskLevel::Medium);
    register_cap("weather.get", "Fetch real live weather conditions for location", contracts::RiskLevel::Low);
    register_cap("web.search", "Execute real live web knowledge search", contracts::RiskLevel::Low);
    register_cap("reminder.create", "Schedule a persistent delayed notification or task", contracts::RiskLevel::Low);
    register_cap("reminder.list", "List active scheduled reminders and jobs", contracts::RiskLevel::Low);
    register_cap("reminder.cancel", "Cancel a scheduled reminder by job ID", contracts::RiskLevel::Low);

    context_->health_service->report_health("lifecycle", observability::HealthStatus::Healthy, "Initialized");
    context_->health_service->report_health("event_bus", observability::HealthStatus::Healthy, "Ready to start");
    context_->health_service->report_health("task_manager", observability::HealthStatus::Healthy, "Ready to start");
    context_->health_service->report_health("capability_registry", observability::HealthStatus::Healthy,
        std::to_string(context_->capability_registry->count()) + " capabilities registered");
    context_->health_service->report_health("tool_gateway", observability::HealthStatus::Healthy, "All managers and adapters linked");
    context_->health_service->report_health("persistent_memory", observability::HealthStatus::Healthy, "Disk store online");
    context_->health_service->report_health("undo_manager", observability::HealthStatus::Healthy, "Reversible transaction stack active");
    context_->health_service->report_health("policy_engine", observability::HealthStatus::Healthy, "Ready");
    context_->health_service->report_health("scheduler", observability::HealthStatus::Healthy, "Ready to start");
    context_->health_service->report_health("watchdog", observability::HealthStatus::Healthy, "Ready to start");

    return contracts::Result<void>::ok();
}

contracts::Result<void> VaniRuntime::start() {
    auto trans_res = context_->lifecycle_manager->transition_to(LifecycleState::Starting);
    if (trans_res.is_err()) {
        return trans_res;
    }

    observability::Logger::instance().info("runtime", "Starting VANI Runtime EventBus, Scheduler & Watchdog...");

    auto eb_res = context_->event_bus->start();
    if (eb_res.is_err()) {
        context_->lifecycle_manager->transition_to(LifecycleState::Failed);
        return eb_res;
    }

    auto sched_res = context_->scheduler->start();
    if (sched_res.is_err()) {
        context_->lifecycle_manager->transition_to(LifecycleState::Failed);
        return sched_res;
    }

    auto wd_res = context_->watchdog->start();
    if (wd_res.is_err()) {
        context_->lifecycle_manager->transition_to(LifecycleState::Failed);
        return wd_res;
    }

    context_->lifecycle_manager->transition_to(LifecycleState::Ready);
    observability::Logger::instance().info("runtime", "VANI Mark 2 Runtime is READY.");

    return contracts::Result<void>::ok();
}

contracts::Result<void> VaniRuntime::shutdown() {
    if (context_->lifecycle_manager->is_terminal()) {
        return contracts::Result<void>::ok();
    }

    context_->lifecycle_manager->transition_to(LifecycleState::Stopping);
    observability::Logger::instance().info("runtime", "Stopping VANI Runtime...");

    context_->watchdog->stop();
    context_->scheduler->stop();
    context_->event_bus->stop();

    context_->lifecycle_manager->transition_to(LifecycleState::Stopped);
    observability::Logger::instance().info("runtime", "VANI Mark 2 Runtime has STOPPED.");

    return contracts::Result<void>::ok();
}

LifecycleState VaniRuntime::state() const noexcept {
    return context_->lifecycle_manager->state();
}

bool VaniRuntime::is_running() const noexcept {
    return context_->lifecycle_manager->is_ready();
}

RuntimeContextPtr VaniRuntime::context() const noexcept { return context_; }
EventBusPtr VaniRuntime::event_bus() const noexcept { return context_->event_bus; }
TaskManagerPtr VaniRuntime::task_manager() const noexcept { return context_->task_manager; }
CapabilityRegistryPtr VaniRuntime::capability_registry() const noexcept { return context_->capability_registry; }
PolicyEnginePtr VaniRuntime::policy_engine() const noexcept { return context_->policy_engine; }
PermissionServicePtr VaniRuntime::permission_service() const noexcept { return context_->permission_service; }
SessionManagerPtr VaniRuntime::session_manager() const noexcept { return context_->session_manager; }
ContextManagerPtr VaniRuntime::context_manager() const noexcept { return context_->context_manager; }
SchedulerPtr VaniRuntime::scheduler() const noexcept { return context_->scheduler; }
WatchdogPtr VaniRuntime::watchdog() const noexcept { return context_->watchdog; }
AuditServicePtr VaniRuntime::audit_service() const noexcept { return context_->audit_service; }
ResourceManagerPtr VaniRuntime::resource_manager() const noexcept { return context_->resource_manager; }
observability::HealthServicePtr VaniRuntime::health_service() const noexcept { return context_->health_service; }
capabilities::system::ToolGatewayPtr VaniRuntime::tool_gateway() const noexcept { return context_->tool_gateway; }

} // namespace vani::runtime
