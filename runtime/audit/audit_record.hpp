#pragma once

#include "../../contracts/common/version.hpp"
#include <string>
#include <chrono>

namespace vani::runtime {

struct AuditRecord {
    std::string record_id;
    uint64_t timestamp_ms{0};
    std::string actor_id;
    contracts::SessionId session_id;
    contracts::TaskId task_id;
    contracts::CapabilityId capability_id;
    std::string action;
    std::string decision;
    std::string result;
    std::string details_json;

    [[nodiscard]] static AuditRecord create(
        std::string actor,
        std::string action,
        std::string decision,
        std::string result,
        std::string capability = "",
        contracts::TaskId task = "",
        contracts::SessionId session = "",
        std::string details = ""
    ) {
        const auto now_ms = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
        return AuditRecord{
            .record_id = "aud_" + std::to_string(now_ms),
            .timestamp_ms = now_ms,
            .actor_id = std::move(actor),
            .session_id = std::move(session),
            .task_id = std::move(task),
            .capability_id = std::move(capability),
            .action = std::move(action),
            .decision = std::move(decision),
            .result = std::move(result),
            .details_json = std::move(details)
        };
    }
};

} // namespace vani::runtime
