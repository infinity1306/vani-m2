#include "../runtime/core/runtime.hpp"
#include <iostream>
#include <chrono>
#include <vector>

int main() {
    std::cout << "========================================================\n";
    std::cout << " VANI Mark 2 — Phase 2 Runtime Performance Benchmarks\n";
    std::cout << "========================================================\n";

    // 1. Benchmark: Runtime Boot & Shutdown
    {
        auto start = std::chrono::high_resolution_clock::now();
        vani::runtime::VaniRuntime runtime;
        runtime.initialize();
        runtime.start();
        auto boot_done = std::chrono::high_resolution_clock::now();
        runtime.shutdown();
        auto shutdown_done = std::chrono::high_resolution_clock::now();

        auto boot_us = std::chrono::duration_cast<std::chrono::microseconds>(boot_done - start).count();
        auto shut_us = std::chrono::duration_cast<std::chrono::microseconds>(shutdown_done - boot_done).count();
        std::cout << "  [BENCHMARK] Runtime Boot:     " << boot_us << " us\n";
        std::cout << "  [BENCHMARK] Runtime Shutdown: " << shut_us << " us\n";
    }

    vani::runtime::VaniRuntime runtime;
    runtime.initialize();
    runtime.start();

    // 2. Benchmark: Task Creation Latency (10,000 tasks)
    {
        constexpr int N = 10000;
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < N; ++i) {
            runtime.task_manager()->create_task({
                .task_id = "",
                .session_id = "sess_bench",
                .parent_task_id = std::nullopt,
                .title = "Benchmark Task",
                .description = "Task load testing",
                .category = "benchmark",
                .priority = vani::contracts::TaskPriority::Normal,
                .requested_capabilities = {"system.echo"},
                .required_permissions = {},
                .assigned_agent_id = "",
                .timeout_seconds = 60
            });
        }
        auto done = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(done - start).count();
        std::cout << "  [BENCHMARK] Task Creation:    " << (double)us / N << " us/task (" << (N * 1000000.0 / us) << " tasks/sec)\n";
    }

    // 3. Benchmark: Policy Evaluation (50,000 evals)
    {
        constexpr int N = 50000;
        vani::runtime::PolicyContext ctx{
            .actor_id = "agent.odysseus",
            .capability_id = "fs.read",
            .risk_level = vani::contracts::RiskLevel::Low,
            .user_id = "default_user",
            .session_id = "sess_bench",
            .task_id = "task_bench",
            .resource_path = "/workspace/src",
            .is_local_execution = true,
            .is_user_present = true,
            .data_is_sensitive = false,
            .metadata = {}
        };

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < N; ++i) {
            volatile auto dec = runtime.policy_engine()->evaluate(ctx);
            (void)dec;
        }
        auto done = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(done - start).count();
        std::cout << "  [BENCHMARK] Policy Eval:      " << (double)us / N << " us/eval (" << (N * 1000000.0 / us) << " evals/sec)\n";
    }

    // 4. Benchmark: Capability Lookup (100,000 lookups)
    {
        runtime.capability_registry()->register_capability({
            .id = "system.echo",
            .version = {1, 0, 0},
            .type = vani::runtime::CapabilityType::Tool,
            .provider_id = "core.system",
            .description = "Echo tool",
            .input_schema_json = "{}",
            .output_schema_json = "{}",
            .required_permissions = {},
            .risk_level = vani::contracts::RiskLevel::Safe,
            .availability = vani::runtime::CapabilityAvailability::Available,
            .requires_network = false,
            .requires_gpu = false,
            .requires_camera = false,
            .requires_microphone = false,
            .supports_streaming = false,
            .supports_cancellation = true,
            .supports_resume = false
        });

        constexpr int N = 100000;
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < N; ++i) {
            auto cap = runtime.capability_registry()->get_capability("system.echo");
            (void)cap;
        }
        auto done = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(done - start).count();
        std::cout << "  [BENCHMARK] Capability Lookup:" << (double)us / N << " us/lookup (" << (N * 1000000.0 / us) << " lookups/sec)\n";
    }

    // 5. Benchmark: Event Bus Dispatch
    {
        constexpr int N = 20000;
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < N; ++i) {
            auto evt = std::make_shared<vani::contracts::Event>(
                vani::contracts::EventHeader{
                    .event_id = "evt_bench",
                    .event_type = "bench.metric",
                    .event_version = {1, 0, 0},
                    .timestamp_ms = 0,
                    .source = "benchmark",
                    .session_id = "sess_0",
                    .task_id = "task_0",
                    .correlation_id = "corr_0",
                    .category = vani::contracts::EventCategory::System
                }
            );
            runtime.event_bus()->publish(evt, vani::runtime::DeliveryGuarantee::Ephemeral, vani::runtime::BackpressureStrategy::DropOldest);
        }
        auto done = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(done - start).count();
        std::cout << "  [BENCHMARK] Event Publish:    " << (double)us / N << " us/event (" << (N * 1000000.0 / us) << " events/sec)\n";
    }

    runtime.shutdown();
    std::cout << "========================================================\n";
    std::cout << " Benchmark Suite Complete.\n";
    std::cout << "========================================================\n";
    return 0;
}
