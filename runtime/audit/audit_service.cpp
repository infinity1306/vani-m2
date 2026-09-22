#include "audit_service.hpp"

namespace vani::runtime {

AuditService::AuditService(
    EventBusPtr event_bus,
    storage::AuditRepositoryPtr repository
) : event_bus_(std::move(event_bus)),
    repository_(std::move(repository)) {}

AuditService::~AuditService() = default;

contracts::Result<void> AuditService::record(
    std::string actor_id,
    std::string action,
    std::string decision,
    std::string result,
    std::string capability_id,
    contracts::TaskId task_id,
    contracts::SessionId session_id,
    std::string details
) {
    auto audit = AuditRecord::create(
        std::move(actor_id),
        std::move(action),
        std::move(decision),
        std::move(result),
        std::move(capability_id),
        task_id,
        session_id,
        std::move(details)
    );

    {
        std::lock_guard<std::mutex> lock(mutex_);
        records_.push_back(audit);
        if (repository_) {
            repository_->save(audit);
        }
    }

    if (event_bus_) {
        auto evt = std::make_shared<contracts::Event>(
            contracts::EventHeader{
                .event_id = "evt_" + audit.record_id,
                .event_type = "audit.recorded",
                .event_version = {1, 0, 0},
                .timestamp_ms = audit.timestamp_ms,
                .source = "vani.runtime.audit",
                .session_id = session_id,
                .task_id = task_id,
                .correlation_id = "corr_" + audit.record_id,
                .category = contracts::EventCategory::System
            }
        );
        event_bus_->publish(evt, DeliveryGuarantee::Durable);
    }

    return contracts::Result<void>::ok();
}

std::vector<AuditRecord> AuditService::list_all() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return records_;
}

std::vector<AuditRecord> AuditService::list_by_task(const contracts::TaskId& task_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AuditRecord> list;
    for (const auto& r : records_) {
        if (r.task_id == task_id) {
            list.push_back(r);
        }
    }
    return list;
}

size_t AuditService::record_count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return records_.size();
}

} // namespace vani::runtime
