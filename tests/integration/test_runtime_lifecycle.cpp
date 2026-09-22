#include "../../runtime/core/runtime.hpp"
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>

void test_runtime_boot_and_shutdown() {
    vani::config::RuntimeProfile profile;
    vani::runtime::VaniRuntime runtime(profile);

    assert(runtime.state() == vani::runtime::LifecycleState::Uninitialized);

    auto init_res = runtime.initialize();
    assert(init_res.is_ok());
    assert(runtime.state() == vani::runtime::LifecycleState::Initializing);

    auto start_res = runtime.start();
    assert(start_res.is_ok());
    assert(runtime.is_running());
    assert(runtime.state() == vani::runtime::LifecycleState::Ready);

    // Verify Event Bus publish/subscribe
    std::atomic<bool> event_received{false};
    auto sub_id = runtime.event_bus()->subscribe(
        vani::runtime::EventFilter{
            .event_type = "test.ping",
            .category = std::nullopt,
            .session_id = std::nullopt,
            .task_id = std::nullopt
        },
        [&](vani::contracts::EventPtr event) {
            if (event && event->event_type() == "test.ping") {
                event_received.store(true);
            }
        }
    );
    assert(sub_id > 0);

    auto evt = std::make_shared<vani::contracts::Event>(
        vani::contracts::EventHeader{
            .event_id = "evt_001",
            .event_type = "test.ping",
            .event_version = {1, 0, 0},
            .timestamp_ms = 0,
            .source = "test",
            .session_id = "sess_001",
            .task_id = "task_001",
            .correlation_id = "corr_001",
            .category = vani::contracts::EventCategory::System
        }
    );
    runtime.event_bus()->publish(evt);

    // Wait briefly for async delivery
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(event_received.load());

    // Verify Task Manager Lifecycle
    auto task_res = runtime.task_manager()->create_task({
        .task_id = "task_lifecycle_001",
        .session_id = "sess_001",
        .parent_task_id = std::nullopt,
        .title = "Compile C++ Engine",
        .description = "Test task compilation",
        .category = "coding",
        .priority = vani::contracts::TaskPriority::High,
        .requested_capabilities = {},
        .required_permissions = {},
        .assigned_agent_id = "",
        .timeout_seconds = 300
    });
    assert(task_res.is_ok());
    const auto task_id = task_res.value();

    auto task_opt = runtime.task_manager()->get_task(task_id);
    assert(task_opt.has_value());
    assert(task_opt->state == vani::contracts::TaskState::Created);

    auto run_res = runtime.task_manager()->transition_task_state(task_id, vani::contracts::TaskState::Running);
    assert(run_res.is_ok());
    assert(runtime.task_manager()->get_task(task_id)->state == vani::contracts::TaskState::Running);

    auto comp_res = runtime.task_manager()->transition_task_state(task_id, vani::contracts::TaskState::Completed);
    assert(comp_res.is_ok());
    assert(runtime.task_manager()->get_task(task_id)->state == vani::contracts::TaskState::Completed);

    // Shutdown
    auto shut_res = runtime.shutdown();
    assert(shut_res.is_ok());
    assert(runtime.state() == vani::runtime::LifecycleState::Terminated);

    std::cout << "  [PASS] test_runtime_boot_and_shutdown\n";
}
