#include "health_service.hpp"
#include <chrono>

namespace vani::observability {

void HealthService::report_health(
    const std::string& component_name,
    HealthStatus status,
    const std::string& message
) {
    const auto now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    std::lock_guard<std::mutex> lock(mutex_);
    health_registry_[component_name] = ComponentHealth{
        .component_name = component_name,
        .status = status,
        .message = message,
        .last_check_timestamp_ms = now_ms
    };
}

ComponentHealth HealthService::get_component_health(const std::string& component_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = health_registry_.find(component_name);
    if (it != health_registry_.end()) {
        return it->second;
    }
    return ComponentHealth{
        .component_name = component_name,
        .status = HealthStatus::Unavailable,
        .message = "Component not registered in health service"
    };
}

std::unordered_map<std::string, ComponentHealth> HealthService::get_all_health() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return health_registry_;
}

bool HealthService::is_system_healthy() const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [name, health] : health_registry_) {
        if (health.status == HealthStatus::Failed || health.status == HealthStatus::Unavailable) {
            return false;
        }
    }
    return true;
}

} // namespace vani::observability
