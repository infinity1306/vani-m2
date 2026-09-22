#pragma once

#include "task_repository.hpp"
#include <unordered_map>
#include <mutex>

namespace vani::storage {

class InMemoryTaskRepository : public ITaskRepository {
public:
    contracts::Result<void> save(const contracts::Task& task) override {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_[task.id()] = task;
        return contracts::Result<void>::ok();
    }

    [[nodiscard]] std::optional<contracts::Task> find_by_id(const contracts::TaskId& id) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tasks_.find(id);
        if (it != tasks_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::vector<contracts::Task> find_all() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<contracts::Task> result;
        result.reserve(tasks_.size());
        for (const auto& [id, task] : tasks_) {
            result.push_back(task);
        }
        return result;
    }

    [[nodiscard]] std::vector<contracts::Task> find_by_session(const contracts::SessionId& session_id) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<contracts::Task> result;
        for (const auto& [id, task] : tasks_) {
            if (task.spec.session_id == session_id) {
                result.push_back(task);
            }
        }
        return result;
    }

    [[nodiscard]] std::vector<contracts::Task> find_by_state(contracts::TaskState state) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<contracts::Task> result;
        for (const auto& [id, task] : tasks_) {
            if (task.state == state) {
                result.push_back(task);
            }
        }
        return result;
    }

    contracts::Result<void> remove(const contracts::TaskId& id) override {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.erase(id);
        return contracts::Result<void>::ok();
    }

    [[nodiscard]] size_t count() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return tasks_.size();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<contracts::TaskId, contracts::Task> tasks_;
};

} // namespace vani::storage
