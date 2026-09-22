#pragma once

#include "repository.hpp"
#include "../runtime/audit/audit_record.hpp"
#include <unordered_map>
#include <mutex>
#include <memory>

namespace vani::storage {

class IAuditRepository : public IRepository<runtime::AuditRecord, std::string> {
public:
    [[nodiscard]] virtual std::vector<runtime::AuditRecord> find_by_task(const contracts::TaskId& task_id) const = 0;
    [[nodiscard]] virtual std::vector<runtime::AuditRecord> find_by_actor(const std::string& actor_id) const = 0;
};

class InMemoryAuditRepository : public IAuditRepository {
public:
    contracts::Result<void> save(const runtime::AuditRecord& record) override {
        std::lock_guard<std::mutex> lock(mutex_);
        records_[record.record_id] = record;
        return contracts::Result<void>::ok();
    }

    [[nodiscard]] std::optional<runtime::AuditRecord> find_by_id(const std::string& id) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = records_.find(id);
        if (it != records_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::vector<runtime::AuditRecord> find_all() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<runtime::AuditRecord> result;
        result.reserve(records_.size());
        for (const auto& [id, rec] : records_) {
            result.push_back(rec);
        }
        return result;
    }

    [[nodiscard]] std::vector<runtime::AuditRecord> find_by_task(const contracts::TaskId& task_id) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<runtime::AuditRecord> result;
        for (const auto& [id, rec] : records_) {
            if (rec.task_id == task_id) {
                result.push_back(rec);
            }
        }
        return result;
    }

    [[nodiscard]] std::vector<runtime::AuditRecord> find_by_actor(const std::string& actor_id) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<runtime::AuditRecord> result;
        for (const auto& [id, rec] : records_) {
            if (rec.actor_id == actor_id) {
                result.push_back(rec);
            }
        }
        return result;
    }

    contracts::Result<void> remove(const std::string& id) override {
        std::lock_guard<std::mutex> lock(mutex_);
        records_.erase(id);
        return contracts::Result<void>::ok();
    }

    [[nodiscard]] size_t count() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return records_.size();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, runtime::AuditRecord> records_;
};

using AuditRepositoryPtr = std::shared_ptr<IAuditRepository>;

} // namespace vani::storage
