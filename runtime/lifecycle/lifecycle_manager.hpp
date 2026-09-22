#pragma once

#include "lifecycle_state.hpp"
#include "../../contracts/common/result.hpp"
#include <string_view>
#include <atomic>
#include <mutex>
#include <functional>
#include <vector>

namespace vani::runtime {

using StateChangeCallback = std::function<void(LifecycleState old_state, LifecycleState new_state)>;

class LifecycleManager {
public:
    LifecycleManager();
    ~LifecycleManager();

    [[nodiscard]] LifecycleState state() const noexcept;
    [[nodiscard]] bool is_ready() const noexcept;
    [[nodiscard]] bool is_stopping() const noexcept;
    [[nodiscard]] bool is_terminal() const noexcept;

    contracts::Result<void> transition_to(LifecycleState next_state);

    void register_state_listener(StateChangeCallback listener);

private:
    std::atomic<LifecycleState> state_;
    std::mutex mutex_;
    std::vector<StateChangeCallback> listeners_;
};

} // namespace vani::runtime
