#pragma once

#include "../../contracts/common/result.hpp"
#include "../event_bus/event_bus.hpp"
#include "../task_manager/task_manager.hpp"
#include "../../observability/health_service.hpp"
#include <unordered_map>
#include <mutex>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

namespace vani::runtime {

struct WorkerHeartbeat {
    std::string worker_id;
    uint64_t last_heartbeat_ms{0};
    bool is_healthy{true};
};

class Watchdog {
public:
    explicit Watchdog(
        TaskManagerPtr task_manager = nullptr,
        observability::HealthServicePtr health_service = nullptr,
        EventBusPtr event_bus = nullptr,
        uint32_t heartbeat_timeout_ms = 10000,
        uint32_t check_interval_ms = 1000
    );
    ~Watchdog();

    contracts::Result<void> start();
    contracts::Result<void> stop();

    void record_heartbeat(const std::string& worker_id);
    void check_system_health(uint64_t current_time_ms);

    [[nodiscard]] size_t monitored_workers_count() const noexcept;

private:
    void loop();

    std::atomic<bool> running_{false};
    uint32_t heartbeat_timeout_ms_;
    uint32_t check_interval_ms_;

    TaskManagerPtr task_manager_;
    observability::HealthServicePtr health_service_;
    EventBusPtr event_bus_;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, WorkerHeartbeat> heartbeats_;
    std::thread worker_;
};

using WatchdogPtr = std::shared_ptr<Watchdog>;

} // namespace vani::runtime
