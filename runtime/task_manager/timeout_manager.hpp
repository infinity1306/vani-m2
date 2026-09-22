#pragma once

#include "../../contracts/tasks/task.hpp"
#include "../../contracts/common/cancellation_token.hpp"
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <functional>

namespace vani::runtime {

struct TaskTimeoutConfig {
    uint32_t soft_timeout_ms{30000};
    uint32_t hard_timeout_ms{60000};
};

using TimeoutCallback = std::function<void(const contracts::TaskId& task_id, bool is_hard)>;

class TimeoutManager {
public:
    TimeoutManager();
    ~TimeoutManager();

    void register_task(
        const contracts::TaskId& task_id,
        TaskTimeoutConfig config,
        contracts::CancellationSource cancellation_source,
        TimeoutCallback on_timeout
    );

    void unregister_task(const contracts::TaskId& task_id);

    void check_timeouts(uint64_t current_time_ms);

private:
    struct TaskTimeoutEntry {
        contracts::TaskId task_id;
        TaskTimeoutConfig config;
        uint64_t start_time_ms;
        bool soft_fired{false};
        contracts::CancellationSource cancellation_source;
        TimeoutCallback on_timeout;
    };

    mutable std::mutex mutex_;
    std::unordered_map<contracts::TaskId, TaskTimeoutEntry> active_timeouts_;
};

} // namespace vani::runtime
