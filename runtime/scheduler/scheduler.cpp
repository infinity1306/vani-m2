#include "scheduler.hpp"
#include <chrono>

namespace vani::runtime {

Scheduler::Scheduler(
    TaskManagerPtr task_manager,
    storage::SchedulerRepositoryPtr repository,
    uint32_t tick_interval_ms
) : tick_interval_ms_(tick_interval_ms),
    task_manager_(std::move(task_manager)),
    repository_(std::move(repository)) {
    if (repository_) {
        for (const auto& job : repository_->find_all()) {
            jobs_[job.job_id] = job;
        }
    }
}

Scheduler::~Scheduler() {
    stop();
}

contracts::Result<void> Scheduler::start() {
    if (running_.exchange(true)) {
        return contracts::Result<void>::ok();
    }

    worker_ = std::thread(&Scheduler::loop, this);
    return contracts::Result<void>::ok();
}

contracts::Result<void> Scheduler::stop() {
    if (!running_.exchange(false)) {
        return contracts::Result<void>::ok();
    }

    cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
    return contracts::Result<void>::ok();
}

contracts::Result<std::string> Scheduler::schedule_now(const contracts::TaskSpecification& task_spec) {
    const auto now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    return schedule_at(task_spec, now_ms);
}

contracts::Result<std::string> Scheduler::schedule_after(
    const contracts::TaskSpecification& task_spec,
    uint64_t delay_ms
) {
    const auto now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    return schedule_at(task_spec, now_ms + delay_ms);
}

contracts::Result<std::string> Scheduler::schedule_at(
    const contracts::TaskSpecification& task_spec,
    uint64_t target_time_ms
) {
    const auto now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    std::lock_guard<std::mutex> lock(mutex_);
    const auto job_id = "job_" + std::to_string(now_ms) + "_" + std::to_string(job_counter_++);

    ScheduledJob job{
        .job_id = job_id,
        .task_spec = task_spec,
        .type = (target_time_ms <= now_ms) ? ScheduleType::RunNow : ScheduleType::RunAt,
        .target_time_ms = target_time_ms,
        .interval_ms = 0,
        .is_recurring = false,
        .is_enabled = true,
        .created_at_ms = now_ms,
        .last_executed_ms = 0
    };

    jobs_[job_id] = job;
    if (repository_) {
        repository_->save(job);
    }

    cv_.notify_one();
    return contracts::Result<std::string>(job_id);
}

contracts::Result<std::string> Scheduler::schedule_recurring(
    const contracts::TaskSpecification& task_spec,
    uint64_t interval_ms,
    uint64_t start_time_ms
) {
    if (interval_ms == 0) {
        return contracts::Result<std::string>::err(
            contracts::ErrorCode::ValidationError,
            "Recurring interval cannot be 0",
            "vani.runtime.scheduler",
            false,
            contracts::ErrorCategory::Validation
        );
    }

    const auto now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    const auto target = (start_time_ms > 0) ? start_time_ms : (now_ms + interval_ms);

    std::lock_guard<std::mutex> lock(mutex_);
    const auto job_id = "job_rec_" + std::to_string(now_ms) + "_" + std::to_string(job_counter_++);

    ScheduledJob job{
        .job_id = job_id,
        .task_spec = task_spec,
        .type = ScheduleType::Recurring,
        .target_time_ms = target,
        .interval_ms = interval_ms,
        .is_recurring = true,
        .is_enabled = true,
        .created_at_ms = now_ms,
        .last_executed_ms = 0
    };

    jobs_[job_id] = job;
    if (repository_) {
        repository_->save(job);
    }

    cv_.notify_one();
    return contracts::Result<std::string>(job_id);
}

contracts::Result<void> Scheduler::cancel_job(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::NotFound,
            "Job not found: " + job_id,
            "vani.runtime.scheduler",
            false,
            contracts::ErrorCategory::Validation
        );
    }

    it->second.is_enabled = false;
    if (repository_) {
        repository_->save(it->second);
    }
    return contracts::Result<void>::ok();
}

std::optional<ScheduledJob> Scheduler::get_job(const std::string& job_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = jobs_.find(job_id);
    if (it != jobs_.end()) {
        return it->second;
    }
    if (repository_) {
        return repository_->find_by_id(job_id);
    }
    return std::nullopt;
}

std::vector<ScheduledJob> Scheduler::list_jobs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ScheduledJob> list;
    list.reserve(jobs_.size());
    for (const auto& [id, j] : jobs_) {
        list.push_back(j);
    }
    return list;
}

size_t Scheduler::pending_job_count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [id, j] : jobs_) {
        if (j.is_enabled) count++;
    }
    return count;
}

void Scheduler::tick(uint64_t current_time_ms) {
    std::vector<ScheduledJob> due_jobs;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [id, job] : jobs_) {
            if (job.is_enabled && job.target_time_ms <= current_time_ms) {
                due_jobs.push_back(job);
                job.last_executed_ms = current_time_ms;

                if (job.is_recurring) {
                    job.target_time_ms = current_time_ms + job.interval_ms;
                } else {
                    job.is_enabled = false;
                }

                if (repository_) {
                    repository_->save(job);
                }
            }
        }
    }

    for (const auto& job : due_jobs) {
        if (task_manager_) {
            task_manager_->create_task(job.task_spec);
        }
    }
}

void Scheduler::loop() {
    while (running_.load(std::memory_order_relaxed)) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, std::chrono::milliseconds(tick_interval_ms_), [this] {
                return !running_.load(std::memory_order_relaxed);
            });
        }

        if (!running_.load(std::memory_order_relaxed)) break;

        const auto now_ms = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
        tick(now_ms);
    }
}

} // namespace vani::runtime
