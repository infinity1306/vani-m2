#include "../../runtime/core/runtime.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>

void test_concurrent_task_creation_and_cancellation() {
    vani::runtime::VaniRuntime runtime;
    runtime.initialize();
    runtime.start();

    constexpr int THREADS = 8;
    constexpr int TASKS_PER_THREAD = 100;
    std::vector<std::thread> workers;
    std::atomic<int> completed_tasks{0};
    std::atomic<int> cancelled_tasks{0};

    for (int t = 0; t < THREADS; ++t) {
        workers.emplace_back([&, t]() {
            for (int i = 0; i < TASKS_PER_THREAD; ++i) {
                const auto task_id = "task_conc_" + std::to_string(t) + "_" + std::to_string(i);
                auto res = runtime.task_manager()->create_task({
                    .task_id = task_id,
                    .session_id = "sess_conc",
                    .parent_task_id = std::nullopt,
                    .title = "Concurrent Task",
                    .description = "Stress test",
                    .category = "stress",
                    .priority = vani::contracts::TaskPriority::Normal,
                    .requested_capabilities = {"system.echo"},
                    .required_permissions = {},
                    .assigned_agent_id = "",
                    .timeout_seconds = 10
                });

                if (res.is_ok()) {
                    if (i % 2 == 0) {
                        runtime.task_manager()->transition_task_state(task_id, vani::contracts::TaskState::Running);
                        runtime.task_manager()->complete_task(task_id, vani::contracts::TaskResult{.output_summary = "Done"});
                        completed_tasks.fetch_add(1);
                    } else {
                        runtime.task_manager()->request_cancel(task_id, "Concurrent cancellation race");
                        cancelled_tasks.fetch_add(1);
                    }
                }
            }
        });
    }

    for (auto& w : workers) {
        if (w.joinable()) w.join();
    }

    assert(completed_tasks.load() == (THREADS * TASKS_PER_THREAD) / 2);
    assert(cancelled_tasks.load() == (THREADS * TASKS_PER_THREAD) / 2);

    runtime.shutdown();
    std::cout << "  [PASS] test_concurrent_task_creation_and_cancellation (" 
              << (THREADS * TASKS_PER_THREAD) << " tasks)\n";
}

void test_event_bus_backpressure_and_flood() {
    vani::runtime::EventBus bus(4, 500); // 4 workers, bounded queue of 500
    bus.start();

    std::atomic<int> received_count{0};
    auto sub_id = bus.subscribe(
        vani::runtime::EventFilter{.event_type = "flood.test"},
        [&](vani::contracts::EventPtr) {
            received_count.fetch_add(1);
        }
    );
    assert(sub_id > 0);

    // Flood 5,000 events into the 500-capacity bounded queue
    constexpr int FLOOD_COUNT = 5000;
    for (int i = 0; i < FLOOD_COUNT; ++i) {
        auto evt = std::make_shared<vani::contracts::Event>(
            vani::contracts::EventHeader{
                .event_id = "evt_flood_" + std::to_string(i),
                .event_type = "flood.test",
                .event_version = {1, 0, 0},
                .timestamp_ms = 0,
                .source = "test",
                .session_id = "sess_flood",
                .task_id = "task_flood",
                .correlation_id = "corr_flood",
                .category = vani::contracts::EventCategory::System
            }
        );
        // Using DropOldest strategy
        bus.publish(evt, vani::runtime::DeliveryGuarantee::Ephemeral, vani::runtime::BackpressureStrategy::DropOldest);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    bus.stop();

    // Verify bounded queue handled load without unbounded growth or crashes
    assert(bus.published_events_count() == FLOOD_COUNT);
    assert(bus.dropped_events_count() > 0); // Proof of active backpressure shedding

    std::cout << "  [PASS] test_event_bus_backpressure_and_flood (Dropped: " 
              << bus.dropped_events_count() << " / " << FLOOD_COUNT << ")\n";
}

int main() {
    std::cout << "--- Running Phase 2 Concurrency & Backpressure Tests ---\n";
    test_concurrent_task_creation_and_cancellation();
    test_event_bus_backpressure_and_flood();
    std::cout << "All Phase 2 Concurrency Tests PASSED.\n";
    return 0;
}
