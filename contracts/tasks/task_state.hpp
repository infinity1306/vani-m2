#pragma once

#include <string_view>
#include <cstdint>

namespace vani::contracts {

enum class TaskState : uint8_t {
    Created,
    Planning,
    Ready,
    WaitingPermission,
    Running,
    WaitingInput,
    Paused,
    Verifying,
    Recovering,
    Completed,
    Failed,
    Cancelled
};

[[nodiscard]] constexpr std::string_view to_string(TaskState state) noexcept {
    switch (state) {
        case TaskState::Created: return "CREATED";
        case TaskState::Planning: return "PLANNING";
        case TaskState::Ready: return "READY";
        case TaskState::WaitingPermission: return "WAITING_PERMISSION";
        case TaskState::Running: return "RUNNING";
        case TaskState::WaitingInput: return "WAITING_INPUT";
        case TaskState::Paused: return "PAUSED";
        case TaskState::Verifying: return "VERIFYING";
        case TaskState::Recovering: return "RECOVERING";
        case TaskState::Completed: return "COMPLETED";
        case TaskState::Failed: return "FAILED";
        case TaskState::Cancelled: return "CANCELLED";
        default: return "UNKNOWN";
    }
}

enum class TaskPriority : uint8_t {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

[[nodiscard]] constexpr std::string_view to_string(TaskPriority priority) noexcept {
    switch (priority) {
        case TaskPriority::Low: return "LOW";
        case TaskPriority::Normal: return "NORMAL";
        case TaskPriority::High: return "HIGH";
        case TaskPriority::Critical: return "CRITICAL";
        default: return "NORMAL";
    }
}

} // namespace vani::contracts
