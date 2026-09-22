#include "watchdog.hpp"

namespace vani::runtime {

Watchdog::Watchdog(
    TaskManagerPtr task_manager,
    observability::HealthServicePtr health_service,
    EventBusPtr event_bus,
    uint32_t heartbeat_timeout_ms,
    uint32_t check_interval_ms
) : heartbeat_timeout_ms_(heartbeat_timeout_ms),
    check_interval_ms_(check_interval_ms),
    task_manager_(std::move(task_manager)),
    health_service_(std::move(health_service)),
    event_bus_(std::move(event_bus)) {}

Watchdog::~Watchdog() {
    stop();
}

contracts::Result<void> Watchdog::start() {
    if (running_.exchange(true)) {
        return contracts::Result<void>::ok();
    }

    worker_ = std::thread(&Watchdog::loop, this);
    return contracts::Result<void>::ok();
}

contracts::Result<void> Watchdog::stop() {
    if (!running_.exchange(false)) {
        return contracts::Result<void>::ok();
    }

    if (worker_.joinable()) {
        worker_.join();
    }
    return contracts::Result<void>::ok();
}

void Watchdog::record_heartbeat(const std::string& worker_id) {
    const auto now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    std::lock_guard<std::mutex> lock(mutex_);
    heartbeats_[worker_id] = WorkerHeartbeat{
        .worker_id = worker_id,
        .last_heartbeat_ms = now_ms,
        .is_healthy = true
    };
}

void Watchdog::check_system_health(uint64_t current_time_ms) {
    std::vector<std::string> dead_workers;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [id, hb] : heartbeats_) {
            if (current_time_ms - hb.last_heartbeat_ms > heartbeat_timeout_ms_) {
                hb.is_healthy = false;
                dead_workers.push_back(id);
            }
        }
    }

    for (const auto& worker_id : dead_workers) {
        if (health_service_) {
            health_service_->report_health(
                worker_id,
                observability::HealthStatus::Degraded,
                "Worker heartbeat timed out"
            );
        }

        if (event_bus_) {
            auto evt = std::make_shared<contracts::Event>(
                contracts::EventHeader{
                    .event_id = "evt_watchdog_" + worker_id + "_" + std::to_string(current_time_ms),
                    .event_type = "worker.health_changed",
                    .event_version = {1, 0, 0},
                    .timestamp_ms = current_time_ms,
                    .source = "vani.runtime.watchdog",
                    .category = contracts::EventCategory::System
                }
            );
            event_bus_->publish(evt, DeliveryGuarantee::Critical);
        }
    }

    if (task_manager_) {
        task_manager_->check_all_timeouts(current_time_ms);
    }
}

size_t Watchdog::monitored_workers_count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return heartbeats_.size();
}

void Watchdog::loop() {
    while (running_.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(check_interval_ms_));
        if (!running_.load(std::memory_order_relaxed)) break;

        const auto now_ms = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
        check_system_health(now_ms);
    }
}

} // namespace vani::runtime
