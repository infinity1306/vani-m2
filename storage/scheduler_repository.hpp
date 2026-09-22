#pragma once

#include "repository.hpp"
#include "../runtime/scheduler/schedule_job.hpp"
#include <unordered_map>
#include <mutex>
#include <memory>

namespace vani::storage {

class ISchedulerRepository : public IRepository<runtime::ScheduledJob, std::string> {
public:
    [[nodiscard]] virtual std::vector<runtime::ScheduledJob> find_due_jobs(uint64_t current_time_ms) const = 0;
};

class InMemorySchedulerRepository : public ISchedulerRepository {
public:
    contracts::Result<void> save(const runtime::ScheduledJob& job) override {
        std::lock_guard<std::mutex> lock(mutex_);
        jobs_[job.job_id] = job;
        return contracts::Result<void>::ok();
    }

    [[nodiscard]] std::optional<runtime::ScheduledJob> find_by_id(const std::string& id) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = jobs_.find(id);
        if (it != jobs_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::vector<runtime::ScheduledJob> find_all() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<runtime::ScheduledJob> result;
        result.reserve(jobs_.size());
        for (const auto& [id, job] : jobs_) {
            result.push_back(job);
        }
        return result;
    }

    [[nodiscard]] std::vector<runtime::ScheduledJob> find_due_jobs(uint64_t current_time_ms) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<runtime::ScheduledJob> result;
        for (const auto& [id, job] : jobs_) {
            if (job.is_enabled && job.target_time_ms <= current_time_ms) {
                result.push_back(job);
            }
        }
        return result;
    }

    contracts::Result<void> remove(const std::string& id) override {
        std::lock_guard<std::mutex> lock(mutex_);
        jobs_.erase(id);
        return contracts::Result<void>::ok();
    }

    [[nodiscard]] size_t count() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return jobs_.size();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, runtime::ScheduledJob> jobs_;
};

using SchedulerRepositoryPtr = std::shared_ptr<ISchedulerRepository>;

} // namespace vani::storage
