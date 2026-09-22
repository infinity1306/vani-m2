#pragma once

#include "runtime_context.hpp"
#include "../../contracts/common/result.hpp"
#include <memory>

namespace vani::runtime {

class VaniRuntime {
public:
    explicit VaniRuntime(const config::RuntimeProfile& profile = config::RuntimeProfile{});
    explicit VaniRuntime(RuntimeContextPtr custom_context);
    ~VaniRuntime();

    contracts::Result<void> initialize();
    contracts::Result<void> start();
    contracts::Result<void> shutdown();

    [[nodiscard]] LifecycleState state() const noexcept;
    [[nodiscard]] bool is_running() const noexcept;

    // Subsystem accessors
    [[nodiscard]] RuntimeContextPtr context() const noexcept;
    [[nodiscard]] EventBusPtr event_bus() const noexcept;
    [[nodiscard]] TaskManagerPtr task_manager() const noexcept;
    [[nodiscard]] CapabilityRegistryPtr capability_registry() const noexcept;
    [[nodiscard]] PolicyEnginePtr policy_engine() const noexcept;
    [[nodiscard]] PermissionServicePtr permission_service() const noexcept;
    [[nodiscard]] SessionManagerPtr session_manager() const noexcept;
    [[nodiscard]] ContextManagerPtr context_manager() const noexcept;
    [[nodiscard]] SchedulerPtr scheduler() const noexcept;
    [[nodiscard]] WatchdogPtr watchdog() const noexcept;
    [[nodiscard]] AuditServicePtr audit_service() const noexcept;
    [[nodiscard]] ResourceManagerPtr resource_manager() const noexcept;
    [[nodiscard]] observability::HealthServicePtr health_service() const noexcept;

private:
    void build_default_context(const config::RuntimeProfile& profile);

    RuntimeContextPtr context_;
};

} // namespace vani::runtime
