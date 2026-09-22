#include "recovery_manager.hpp"

namespace vani::runtime {

RecoveryManager::RecoveryManager(RetryPolicy default_policy)
    : policy_(default_policy) {}

RecoveryDecision RecoveryManager::evaluate_recovery(
    const contracts::Task& task,
    uint32_t attempt_count,
    const contracts::Error& error
) const {
    if (error.code == contracts::ErrorCode::Cancelled) {
        return RecoveryDecision{
            .action = RecoveryAction::FailPermanently,
            .delay_ms = 0,
            .alternate_capability_id = "",
            .rationale = "Task was explicitly cancelled by user or system"
        };
    }

    if (error.code == contracts::ErrorCode::PermissionDenied) {
        return RecoveryDecision{
            .action = RecoveryAction::AskUser,
            .delay_ms = 0,
            .alternate_capability_id = "",
            .rationale = "Execution failed due to missing permission; requesting user grant"
        };
    }

    if (policy_.can_retry(attempt_count, error.code, error.category)) {
        const auto delay = policy_.compute_delay_ms(attempt_count + 1);
        return RecoveryDecision{
            .action = RecoveryAction::Retry,
            .delay_ms = delay,
            .alternate_capability_id = "",
            .rationale = "Transient failure, retrying attempt " + std::to_string(attempt_count + 1)
        };
    }

    if (task.checkpoint.has_value() && !task.checkpoint->empty()) {
        return RecoveryDecision{
            .action = RecoveryAction::ResumeCheckpoint,
            .delay_ms = 0,
            .alternate_capability_id = "",
            .rationale = "Retries exhausted; attempting resume from valid checkpoint"
        };
    }

    return RecoveryDecision{
        .action = RecoveryAction::FailPermanently,
        .delay_ms = 0,
        .alternate_capability_id = "",
        .rationale = "Non-retryable error or maximum attempts exceeded: " + error.message
    };
}

} // namespace vani::runtime
