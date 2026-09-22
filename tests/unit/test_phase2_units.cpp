#include "../../runtime/core/runtime.hpp"
#include "../../capabilities/system/system_echo_executor.hpp"
#include "../../capabilities/system/system_time_executor.hpp"
#include <cassert>
#include <iostream>

void test_task_state_invariants() {
    vani::runtime::VaniRuntime runtime;
    runtime.initialize();
    runtime.start();

    auto task_res = runtime.task_manager()->create_task({
        .task_id = "task_test_fsm",
        .session_id = "sess_01",
        .parent_task_id = std::nullopt,
        .title = "FSM Test",
        .description = "State invariant checks",
        .category = "testing",
        .priority = vani::contracts::TaskPriority::Normal,
        .requested_capabilities = {"system.echo"},
        .required_permissions = {},
        .assigned_agent_id = "",
        .timeout_seconds = 30
    });
    assert(task_res.is_ok());
    const auto id = task_res.value();

    auto t = runtime.task_manager()->get_task(id);
    assert(t.has_value());
    assert(t->state == vani::contracts::TaskState::Created);

    // Valid transitions
    assert(runtime.task_manager()->transition_task_state(id, vani::contracts::TaskState::Planning).is_ok());
    assert(runtime.task_manager()->transition_task_state(id, vani::contracts::TaskState::Ready).is_ok());
    assert(runtime.task_manager()->transition_task_state(id, vani::contracts::TaskState::Running).is_ok());
    assert(runtime.task_manager()->transition_task_state(id, vani::contracts::TaskState::Verifying).is_ok());
    assert(runtime.task_manager()->complete_task(id, vani::contracts::TaskResult{.output_summary = "Done"}).is_ok());

    // Illegal transition from terminal Completed state to Running must fail
    auto illegal = runtime.task_manager()->transition_task_state(id, vani::contracts::TaskState::Running);
    assert(illegal.is_err());
    assert(illegal.error().code == vani::contracts::ErrorCode::Failure);

    runtime.shutdown();
    std::cout << "  [PASS] test_task_state_invariants\n";
}

void test_scoped_context_isolation() {
    vani::runtime::ContextManager ctx;

    // Set Task-scoped context
    ctx.set(vani::runtime::ContextScopeType::Task, "task_1", "branch_name", "feat/phase2");
    ctx.set(vani::runtime::ContextScopeType::Task, "task_2", "branch_name", "fix/bugfix");

    // Set Session-scoped context
    ctx.set(vani::runtime::ContextScopeType::Session, "sess_A", "user_locale", "en-US");

    // Scope isolation: task_1 cannot see task_2's value
    assert(ctx.get(vani::runtime::ContextScopeType::Task, "task_1", "branch_name").value() == "feat/phase2");
    assert(ctx.get(vani::runtime::ContextScopeType::Task, "task_2", "branch_name").value() == "fix/bugfix");

    // Missing key in different scope
    assert(!ctx.get(vani::runtime::ContextScopeType::Task, "task_1", "user_locale").has_value());
    assert(ctx.get(vani::runtime::ContextScopeType::Session, "sess_A", "user_locale").value() == "en-US");

    std::cout << "  [PASS] test_scoped_context_isolation\n";
}

void test_recovery_manager_decisions() {
    vani::runtime::RecoveryManager recovery;
    vani::contracts::Task task;

    // Transient network error -> Retry
    auto dec1 = recovery.evaluate_recovery(
        task, 1,
        vani::contracts::Error::make(vani::contracts::ErrorCode::NetworkError, "Connection reset", "network", true, vani::contracts::ErrorCategory::Network)
    );
    assert(dec1.action == vani::runtime::RecoveryAction::Retry);
    assert(dec1.delay_ms > 0);

    // Permission error -> AskUser
    auto dec2 = recovery.evaluate_recovery(
        task, 1,
        vani::contracts::Error::make(vani::contracts::ErrorCode::PermissionDenied, "No write access", "permission", false, vani::contracts::ErrorCategory::Permission)
    );
    assert(dec2.action == vani::runtime::RecoveryAction::AskUser);

    // Explicitly cancelled -> FailPermanently
    auto dec3 = recovery.evaluate_recovery(
        task, 1,
        vani::contracts::Error::make(vani::contracts::ErrorCode::Cancelled, "Cancelled", "core", false, vani::contracts::ErrorCategory::Cancelled)
    );
    assert(dec3.action == vani::runtime::RecoveryAction::FailPermanently);

    std::cout << "  [PASS] test_recovery_manager_decisions\n";
}

int main() {
    std::cout << "--- Running Phase 2 Unit Tests ---\n";
    test_task_state_invariants();
    test_scoped_context_isolation();
    test_recovery_manager_decisions();
    std::cout << "All Phase 2 Unit Tests PASSED.\n";
    return 0;
}
