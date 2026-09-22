#include "../../runtime/core/runtime.hpp"
#include "../../capabilities/system/system_echo_executor.hpp"
#include "../../capabilities/system/system_time_executor.hpp"
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>

void test_reference_flow_system_echo() {
    vani::runtime::VaniRuntime runtime;
    runtime.initialize();
    runtime.start();

    // 1. Session creation
    auto sess_res = runtime.session_manager()->create_session("user_youri", "desktop_primary");
    assert(sess_res.is_ok());
    const auto session_id = sess_res.value();

    // 2. Register capability & executor
    vani::capabilities::SystemEchoExecutor echo_executor;
    const auto echo_manifest = echo_executor.manifest();

    auto reg_res = runtime.capability_registry()->register_capability({
        .id = echo_manifest.id,
        .version = echo_manifest.version,
        .type = vani::runtime::CapabilityType::Tool,
        .provider_id = "core.system",
        .description = echo_manifest.description,
        .input_schema_json = echo_manifest.input_schema_json,
        .output_schema_json = echo_manifest.output_schema_json,
        .required_permissions = echo_manifest.required_permissions,
        .risk_level = echo_manifest.risk_level,
        .availability = vani::runtime::CapabilityAvailability::Available,
        .requires_network = false,
        .requires_gpu = false,
        .requires_camera = false,
        .requires_microphone = false,
        .supports_streaming = false,
        .supports_cancellation = true,
        .supports_resume = false
    });
    assert(reg_res.is_ok());

    // 3. Task creation
    auto task_res = runtime.task_manager()->create_task({
        .task_id = "task_ref_echo_01",
        .session_id = session_id,
        .parent_task_id = std::nullopt,
        .title = "Echo Test Message",
        .description = "Reference capability execution",
        .category = "system",
        .priority = vani::contracts::TaskPriority::Normal,
        .requested_capabilities = {"system.echo"},
        .required_permissions = {},
        .assigned_agent_id = "",
        .timeout_seconds = 30
    });
    assert(task_res.is_ok());
    const auto task_id = task_res.value();
    runtime.session_manager()->add_task_to_session(session_id, task_id);

    // 4. Capability discovery
    auto cap_opt = runtime.capability_registry()->get_capability("system.echo");
    assert(cap_opt.has_value());
    assert(cap_opt->availability == vani::runtime::CapabilityAvailability::Available);

    // 5. Policy evaluation
    vani::runtime::PolicyContext policy_ctx{
        .actor_id = "system.executor",
        .capability_id = "system.echo",
        .risk_level = cap_opt->risk_level,
        .user_id = "user_youri",
        .session_id = session_id,
        .task_id = task_id,
        .resource_path = "",
        .is_local_execution = true,
        .is_user_present = true,
        .data_is_sensitive = false,
        .metadata = {}
    };
    auto decision = runtime.policy_engine()->evaluate(policy_ctx);
    assert(decision == vani::runtime::PolicyDecision::Allow);

    // 6. Transition to Running & Progress update
    assert(runtime.task_manager()->transition_task_state(task_id, vani::contracts::TaskState::Running).is_ok());
    assert(runtime.task_manager()->update_progress(task_id, 50, "Executing system.echo").is_ok());

    // 7. Executor Invocation
    auto token = runtime.task_manager()->get_cancellation_token(task_id);
    auto exec_res = echo_executor.execute({
        .tool_id = "system.echo",
        .task_id = task_id,
        .correlation_id = "corr_echo_01",
        .arguments_json = "{\"message\":\"Hello VANI Mark 2\"}",
        .execution_context = {},
        .cancellation_token = token
    });
    assert(exec_res.is_ok());
    assert(exec_res.value().success);

    // 8. Verifying & Completion
    assert(runtime.task_manager()->transition_task_state(task_id, vani::contracts::TaskState::Verifying).is_ok());
    assert(runtime.task_manager()->complete_task(task_id, vani::contracts::TaskResult{
        .output_summary = "Echo succeeded",
        .structured_data_json = exec_res.value().output_json
    }).is_ok());

    // 9. Immutable Audit record
    assert(runtime.audit_service()->record(
        "system.executor",
        "EXECUTE_TOOL",
        vani::runtime::to_string(decision).data(),
        "SUCCESS",
        "system.echo",
        task_id,
        session_id,
        "Payload: " + exec_res.value().output_json
    ).is_ok());

    assert(runtime.audit_service()->record_count() >= 1);
    assert(runtime.task_manager()->get_task(task_id)->state == vani::contracts::TaskState::Completed);

    runtime.shutdown();
    std::cout << "  [PASS] test_reference_flow_system_echo\n";
}

void test_extensibility_system_time() {
    vani::runtime::VaniRuntime runtime;
    runtime.initialize();
    runtime.start();

    // Adding NEW capability system.time with ZERO core runtime modifications
    vani::capabilities::SystemTimeExecutor time_executor;
    const auto time_manifest = time_executor.manifest();

    auto reg_res = runtime.capability_registry()->register_capability({
        .id = time_manifest.id,
        .version = time_manifest.version,
        .type = vani::runtime::CapabilityType::Tool,
        .provider_id = "core.system.time",
        .description = time_manifest.description,
        .input_schema_json = time_manifest.input_schema_json,
        .output_schema_json = time_manifest.output_schema_json,
        .required_permissions = {},
        .risk_level = time_manifest.risk_level,
        .availability = vani::runtime::CapabilityAvailability::Available,
        .requires_network = false,
        .requires_gpu = false,
        .requires_camera = false,
        .requires_microphone = false,
        .supports_streaming = false,
        .supports_cancellation = true,
        .supports_resume = false
    });
    assert(reg_res.is_ok());

    auto cap = runtime.capability_registry()->get_capability("system.time");
    assert(cap.has_value());

    auto exec_res = time_executor.execute({
        .tool_id = "system.time",
        .task_id = "task_time_01",
        .correlation_id = "corr_time_01",
        .arguments_json = "{}",
        .execution_context = {},
        .cancellation_token = vani::contracts::CancellationToken::none()
    });
    assert(exec_res.is_ok());
    assert(exec_res.value().success);
    assert(!exec_res.value().output_json.empty());

    runtime.shutdown();
    std::cout << "  [PASS] test_extensibility_system_time\n";
}

void test_human_confirmation_workflow() {
    vani::runtime::VaniRuntime runtime;
    runtime.initialize();
    runtime.start();

    // High risk capability triggers RequireConfirmation
    vani::runtime::PolicyContext high_risk_ctx{
        .actor_id = "agent.odysseus",
        .capability_id = "terminal.exec",
        .risk_level = vani::contracts::RiskLevel::High,
        .user_id = "user_youri",
        .session_id = "sess_01",
        .task_id = "task_sensitive_01",
        .resource_path = "/usr/bin",
        .is_local_execution = true,
        .is_user_present = true,
        .data_is_sensitive = true,
        .metadata = {}
    };

    auto decision = runtime.policy_engine()->evaluate(high_risk_ctx);
    assert(decision == vani::runtime::PolicyDecision::RequireConfirmation);

    auto task_res = runtime.task_manager()->create_task({
        .task_id = "task_sensitive_01",
        .session_id = "sess_01",
        .parent_task_id = std::nullopt,
        .title = "Execute Script",
        .description = "High risk operation",
        .category = "terminal",
        .priority = vani::contracts::TaskPriority::High,
        .requested_capabilities = {"terminal.exec"},
        .required_permissions = {"terminal.execute"},
        .assigned_agent_id = "agent.odysseus",
        .timeout_seconds = 60
    });
    assert(task_res.is_ok());

    // Task transitions to WAITING_PERMISSION
    assert(runtime.task_manager()->transition_task_state("task_sensitive_01", vani::contracts::TaskState::WaitingPermission).is_ok());
    assert(runtime.task_manager()->get_task("task_sensitive_01")->state == vani::contracts::TaskState::WaitingPermission);

    // User approves: Grant permission and resume task to RUNNING
    runtime.permission_service()->grant_permission(
        "agent.odysseus",
        "terminal.execute",
        vani::runtime::PermissionScope::Task,
        "sess_01",
        "task_sensitive_01",
        0,
        "User confirmed in confirmation dialog"
    );

    assert(runtime.permission_service()->has_permission("agent.odysseus", "terminal.execute", "sess_01", "task_sensitive_01"));
    assert(runtime.task_manager()->transition_task_state("task_sensitive_01", vani::contracts::TaskState::Running).is_ok());
    assert(runtime.task_manager()->complete_task("task_sensitive_01", vani::contracts::TaskResult{.output_summary = "Executed with user authorization"}).is_ok());

    runtime.shutdown();
    std::cout << "  [PASS] test_human_confirmation_workflow\n";
}

int main() {
    std::cout << "--- Running Phase 2 Reference & Integration Tests ---\n";
    test_reference_flow_system_echo();
    test_extensibility_system_time();
    test_human_confirmation_workflow();
    std::cout << "All Phase 2 Integration Tests PASSED.\n";
    return 0;
}
