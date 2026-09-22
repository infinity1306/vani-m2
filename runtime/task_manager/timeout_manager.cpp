#include "timeout_manager.hpp"

namespace vani::runtime {

TimeoutManager::TimeoutManager() = default;
TimeoutManager::~TimeoutManager() = default;

void TimeoutManager::register_task(
    const contracts::TaskId& task_id,
    TaskTimeoutConfig config,
    contracts::CancellationSource cancellation_source,
    TimeoutCallback on_timeout
) {
    const auto now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    std::lock_guard<std::mutex> lock(mutex_);
    active_timeouts_[task_id] = TaskTimeoutEntry{
        .task_id = task_id,
        .config = config,
        .start_time_ms = now_ms,
        .soft_fired = false,
        .cancellation_source = cancellation_source,
        .on_timeout = std::move(on_timeout)
    };
}

void TimeoutManager::unregister_task(const contracts::TaskId& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    active_timeouts_.erase(task_id);
}

void TimeoutManager::check_timeouts(uint64_t current_time_ms) {
    std::vector<std::pair<contracts::TaskId, bool>> triggered;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [id, entry] : active_timeouts_) {
            const auto elapsed = current_time_ms - entry.start_time_ms;

            if (elapsed >= entry.config.hard_timeout_ms) {
                entry.cancellation_source.cancel();
                triggered.emplace_back(id, true);
            } else if (!entry.soft_fired && elapsed >= entry.config.soft_timeout_ms) {
                entry.soft_fired = true;
                triggered.emplace_back(id, false);
            }
        }
    }

    for (const auto& [id, is_hard] : triggered) {
        TimeoutCallback cb;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = active_timeouts_.find(id);
            if (it != active_timeouts_.end()) {
                cb = it->second.on_timeout;
                if (is_hard) {
                    active_timeouts_.erase(it);
                }
            }
        }
        if (cb) {
            cb(id, is_hard);
        }
    }
}

} // namespace vani::runtime
