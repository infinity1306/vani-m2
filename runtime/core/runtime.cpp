#include "runtime.hpp"
#include "../../storage/in_memory_task_repository.hpp"
#include "../../storage/in_memory_session_repository.hpp"
#include "../../storage/in_memory_audit_repository.hpp"
#include "../../storage/in_memory_scheduler_repository.hpp"
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
    context_->scheduler_repository = std::make_shared<storage::InMemorySchedulerRepository>();

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
}

contracts::Result<void> VaniRuntime::initialize() {
    auto trans_res = context_->lifecycle_manager->transition_to(LifecycleState::Initializing);
    if (trans_res.is_err()) {
        return trans_res;
    }

    observability::Logger::instance().info("runtime", "Initializing VANI Runtime Subsystems...");

    context_->health_service->report_health("lifecycle", observability::HealthStatus::Healthy, "Initialized");
    context_->health_service->report_health("event_bus", observability::HealthStatus::Healthy, "Ready to start");
    context_->health_service->report_health("task_manager", observability::HealthStatus::Healthy, "Ready to start");
    context_->health_service->report_health("capability_registry", observability::HealthStatus::Healthy, "Ready");
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

} // namespace vani::runtime
