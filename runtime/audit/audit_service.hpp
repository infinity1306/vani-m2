#pragma once

#include "audit_record.hpp"
#include "../../contracts/common/result.hpp"
#include "../event_bus/event_bus.hpp"
#include "../../storage/audit_repository.hpp"
#include <mutex>
#include <memory>
#include <vector>

namespace vani::runtime {

class AuditService {
public:
    explicit AuditService(
        EventBusPtr event_bus = nullptr,
        storage::AuditRepositoryPtr repository = nullptr
    );
    ~AuditService();

    contracts::Result<void> record(
        std::string actor_id,
        std::string action,
        std::string decision,
        std::string result,
        std::string capability_id = "",
        contracts::TaskId task_id = "",
        contracts::SessionId session_id = "",
        std::string details = ""
    );

    [[nodiscard]] std::vector<AuditRecord> list_all() const;
    [[nodiscard]] std::vector<AuditRecord> list_by_task(const contracts::TaskId& task_id) const;
    [[nodiscard]] size_t record_count() const noexcept;

private:
    mutable std::mutex mutex_;
    EventBusPtr event_bus_;
    storage::AuditRepositoryPtr repository_;
    std::vector<AuditRecord> records_;
};

using AuditServicePtr = std::shared_ptr<AuditService>;

} // namespace vani::runtime
