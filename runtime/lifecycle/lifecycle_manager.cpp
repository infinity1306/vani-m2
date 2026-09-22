#include "lifecycle_manager.hpp"

namespace vani::runtime {

LifecycleManager::LifecycleManager()
    : state_(LifecycleState::Created) {}

LifecycleManager::~LifecycleManager() = default;

LifecycleState LifecycleManager::state() const noexcept {
    return state_.load(std::memory_order_acquire);
}

bool LifecycleManager::is_ready() const noexcept {
    const auto s = state();
    return s == LifecycleState::Ready || s == LifecycleState::Degraded;
}

bool LifecycleManager::is_stopping() const noexcept {
    const auto s = state();
    return s == LifecycleState::Stopping || s == LifecycleState::Stopped || s == LifecycleState::Failed;
}

bool LifecycleManager::is_terminal() const noexcept {
    const auto s = state();
    return s == LifecycleState::Stopped || s == LifecycleState::Failed;
}

contracts::Result<void> LifecycleManager::transition_to(LifecycleState next_state) {
    std::unique_lock<std::mutex> lock(mutex_);
    const auto current = state_.load(std::memory_order_relaxed);

    bool valid = false;
    switch (current) {
        case LifecycleState::Created:
            valid = (next_state == LifecycleState::Initializing || next_state == LifecycleState::Failed);
            break;
        case LifecycleState::Initializing:
            valid = (next_state == LifecycleState::Starting || next_state == LifecycleState::Stopping || next_state == LifecycleState::Failed);
            break;
        case LifecycleState::Starting:
            valid = (next_state == LifecycleState::Ready || next_state == LifecycleState::Degraded || next_state == LifecycleState::Stopping || next_state == LifecycleState::Failed);
            break;
        case LifecycleState::Ready:
            valid = (next_state == LifecycleState::Degraded || next_state == LifecycleState::Stopping || next_state == LifecycleState::Failed);
            break;
        case LifecycleState::Degraded:
            valid = (next_state == LifecycleState::Ready || next_state == LifecycleState::Stopping || next_state == LifecycleState::Failed);
            break;
        case LifecycleState::Stopping:
            valid = (next_state == LifecycleState::Stopped || next_state == LifecycleState::Failed);
            break;
        case LifecycleState::Stopped:
        case LifecycleState::Failed:
            valid = false;
            break;
    }

    if (!valid) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::Failure,
            "Invalid lifecycle transition from " + std::string(to_string(current)) +
            " to " + std::string(to_string(next_state)),
            "vani.runtime.lifecycle",
            false,
            contracts::ErrorCategory::Internal
        );
    }

    state_.store(next_state, std::memory_order_release);

    for (const auto& listener : listeners_) {
        if (listener) {
            listener(current, next_state);
        }
    }

    return contracts::Result<void>::ok();
}

void LifecycleManager::register_state_listener(StateChangeCallback listener) {
    std::lock_guard<std::mutex> lock(mutex_);
    listeners_.push_back(std::move(listener));
}

} // namespace vani::runtime
