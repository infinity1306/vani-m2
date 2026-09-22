#pragma once

#include "schedule_job.hpp"
#include "../../contracts/common/result.hpp"
#include "../task_manager/task_manager.hpp"
#include "../../storage/scheduler_repository.hpp"
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>
#include <optional>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace vani::runtime {

class Scheduler {
public:
    explicit Scheduler(
        TaskManagerPtr task_manager = nullptr,
        storage::SchedulerRepositoryPtr repository = nullptr,
        uint32_t tick_interval_ms = 100
    );
    ~Scheduler();

    contracts::Result<void> start();
    contracts::Result<void> stop();

    contracts::Result<std::string> schedule_now(
        const contracts::TaskSpecification& task_spec
    );

    contracts::Result<std::string> schedule_at(
        const contracts::TaskSpecification& task_spec,
        uint64_t target_time_ms
    );

    contracts::Result<std::string> schedule_after(
        const contracts::TaskSpecification& task_spec,
        uint64_t delay_ms
    );

    contracts::Result<std::string> schedule_recurring(
        const contracts::TaskSpecification& task_spec,
        uint64_t interval_ms,
        uint64_t start_time_ms = 0
    );

    contracts::Result<void> cancel_job(const std::string& job_id);

    [[nodiscard]] std::optional<ScheduledJob> get_job(const std::string& job_id) const;
    [[nodiscard]] std::vector<ScheduledJob> list_jobs() const;
    [[nodiscard]] size_t pending_job_count() const noexcept;

    void tick(uint64_t current_time_ms);

private:
    void loop();

    std::atomic<bool> running_{false};
    uint32_t tick_interval_ms_;
    TaskManagerPtr task_manager_;
    storage::SchedulerRepositoryPtr repository_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::unordered_map<std::string, ScheduledJob> jobs_;
    std::thread worker_;
    uint64_t job_counter_{1};
};

using SchedulerPtr = std::shared_ptr<Scheduler>;

} // namespace vani::runtime
