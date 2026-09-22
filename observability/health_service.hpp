#pragma once

#include "../contracts/common/result.hpp"
#include <string>
#include <string_view>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace vani::observability {

enum class HealthStatus : uint8_t {
    Healthy,
    Degraded,
    Unavailable,
    Starting,
    Stopping,
    Failed
};

[[nodiscard]] constexpr std::string_view to_string(HealthStatus status) noexcept {
    switch (status) {
        case HealthStatus::Healthy: return "HEALTHY";
        case HealthStatus::Degraded: return "DEGRADED";
        case HealthStatus::Unavailable: return "UNAVAILABLE";
        case HealthStatus::Starting: return "STARTING";
        case HealthStatus::Stopping: return "STOPPING";
        case HealthStatus::Failed: return "FAILED";
        default: return "UNKNOWN";
    }
}

struct ComponentHealth {
    std::string component_name;
    HealthStatus status{HealthStatus::Healthy};
    std::string message;
    uint64_t last_check_timestamp_ms{0};
};

class HealthService {
public:
    void report_health(
        const std::string& component_name,
        HealthStatus status,
        const std::string& message = ""
    );

    [[nodiscard]] ComponentHealth get_component_health(const std::string& component_name) const;

    [[nodiscard]] std::unordered_map<std::string, ComponentHealth> get_all_health() const;

    [[nodiscard]] bool is_system_healthy() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ComponentHealth> health_registry_;
};

using HealthServicePtr = std::shared_ptr<HealthService>;

} // namespace vani::observability
