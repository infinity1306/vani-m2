#pragma once

#include "../../contracts/tasks/task.hpp"
#include "../../contracts/common/result.hpp"
#include "retry_policy.hpp"
#include <string>
#include <memory>

namespace vani::runtime {

enum class RecoveryAction : uint8_t {
    Retry,
    AlternateCapability,
    ResumeCheckpoint,
    Replan,
    AskUser,
    FailPermanently
};

[[nodiscard]] constexpr std::string_view to_string(RecoveryAction action) noexcept {
    switch (action) {
        case RecoveryAction::Retry: return "RETRY";
        case RecoveryAction::AlternateCapability: return "ALTERNATE_CAPABILITY";
        case RecoveryAction::ResumeCheckpoint: return "RESUME_CHECKPOINT";
        case RecoveryAction::Replan: return "REPLAN";
        case RecoveryAction::AskUser: return "ASK_USER";
        case RecoveryAction::FailPermanently: return "FAIL_PERMANENTLY";
        default: return "FAIL_PERMANENTLY";
    }
}

struct RecoveryDecision {
    RecoveryAction action{RecoveryAction::FailPermanently};
    uint32_t delay_ms{0};
    std::string alternate_capability_id;
    std::string rationale;
};

class RecoveryManager {
public:
    explicit RecoveryManager(RetryPolicy default_policy = RetryPolicy{});

    [[nodiscard]] RecoveryDecision evaluate_recovery(
        const contracts::Task& task,
        uint32_t attempt_count,
        const contracts::Error& error
    ) const;

private:
    RetryPolicy policy_;
};

using RecoveryManagerPtr = std::shared_ptr<RecoveryManager>;

} // namespace vani::runtime
