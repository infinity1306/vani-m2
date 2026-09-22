#pragma once

#include "../../config/config_manager.hpp"
#include "../../observability/logger.hpp"
#include "../../observability/health_service.hpp"
#include "../lifecycle/lifecycle_manager.hpp"
#include "../event_bus/event_bus.hpp"
#include "../task_manager/task_manager.hpp"
#include "../capability_registry/capability_registry.hpp"
#include "../policy/policy_engine.hpp"
#include "../permissions/permission_service.hpp"
#include "../session/session_manager.hpp"
#include "../context/context_manager.hpp"
#include "../scheduler/scheduler.hpp"
#include "../watchdog/watchdog.hpp"
#include "../audit/audit_service.hpp"
#include "resource_manager.hpp"
#include "../../storage/task_repository.hpp"
#include "../../storage/session_repository.hpp"
#include "../../storage/audit_repository.hpp"
#include "../../storage/scheduler_repository.hpp"
#include <memory>

namespace vani::runtime {

struct RuntimeContext {
    config::ConfigManagerPtr config_manager;
    observability::HealthServicePtr health_service;
    std::shared_ptr<LifecycleManager> lifecycle_manager;
    EventBusPtr event_bus;
    TaskManagerPtr task_manager;
    CapabilityRegistryPtr capability_registry;
    PolicyEnginePtr policy_engine;
    PermissionServicePtr permission_service;
    SessionManagerPtr session_manager;
    ContextManagerPtr context_manager;
    SchedulerPtr scheduler;
    WatchdogPtr watchdog;
    AuditServicePtr audit_service;
    ResourceManagerPtr resource_manager;

    storage::TaskRepositoryPtr task_repository;
    storage::SessionRepositoryPtr session_repository;
    storage::AuditRepositoryPtr audit_repository;
    storage::SchedulerRepositoryPtr scheduler_repository;
};

using RuntimeContextPtr = std::shared_ptr<RuntimeContext>;

} // namespace vani::runtime
