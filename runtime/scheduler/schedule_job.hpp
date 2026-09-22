#pragma once

#include "../../contracts/tasks/task.hpp"
#include <string>
#include <chrono>

namespace vani::runtime {

enum class ScheduleType : uint8_t {
    RunNow,
    RunAt,
    RunAfter,
    Recurring
};

[[nodiscard]] constexpr std::string_view to_string(ScheduleType type) noexcept {
    switch (type) {
        case ScheduleType::RunNow: return "RUN_NOW";
        case ScheduleType::RunAt: return "RUN_AT";
        case ScheduleType::RunAfter: return "RUN_AFTER";
        case ScheduleType::Recurring: return "RECURRING";
        default: return "UNKNOWN";
    }
}

struct ScheduledJob {
    std::string job_id;
    contracts::TaskSpecification task_spec;
    ScheduleType type{ScheduleType::RunNow};
    uint64_t target_time_ms{0};
    uint64_t interval_ms{0};
    bool is_recurring{false};
    bool is_enabled{true};
    uint64_t created_at_ms{0};
    uint64_t last_executed_ms{0};
};

} // namespace vani::runtime
