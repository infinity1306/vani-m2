#pragma once

#include <string_view>
#include <cstdint>

namespace vani::runtime {

enum class LifecycleState : uint8_t {
    Created,
    Initializing,
    Starting,
    Ready,
    Degraded,
    Stopping,
    Stopped,
    Failed
};

[[nodiscard]] constexpr std::string_view to_string(LifecycleState state) noexcept {
    switch (state) {
        case LifecycleState::Created: return "CREATED";
        case LifecycleState::Initializing: return "INITIALIZING";
        case LifecycleState::Starting: return "STARTING";
        case LifecycleState::Ready: return "READY";
        case LifecycleState::Degraded: return "DEGRADED";
        case LifecycleState::Stopping: return "STOPPING";
        case LifecycleState::Stopped: return "STOPPED";
        case LifecycleState::Failed: return "FAILED";
        default: return "UNKNOWN";
    }
}

} // namespace vani::runtime
