#include "../capabilities/system/gateway/tool_gateway.hpp"
#include "../adapters/system/mock/mock_system_adapter.hpp"
#include "../runtime/policy/policy_engine.hpp"
#include "../runtime/permissions/permission_service.hpp"
#include "../runtime/event_bus/event_bus.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>

using namespace vani::capabilities::system;
using namespace vani::adapters::system;
using namespace vani::contracts;
using namespace vani::runtime;

int main() {
    std::cout << "\n=======================================================\n";
    std::cout << "   VANI Mark 2 — Phase 4 Capabilities Benchmark Suite\n";
    std::cout << "=======================================================\n\n";

    auto mock_adapter = std::make_shared<MockSystemAdapter>();
    auto policy = std::make_shared<PolicyEngine>();
    auto perm = std::make_shared<PermissionService>();
    auto bus = std::make_shared<EventBus>();
    auto audit = std::make_shared<AuditService>();

    auto app_reg = std::make_shared<ApplicationRegistry>();
    auto app_mgr = std::make_shared<ApplicationManager>(mock_adapter, app_reg);
    auto proc_mgr = std::make_shared<ProcessManager>(mock_adapter);
    auto tx_mgr = std::make_shared<FileTransactionManager>();
    auto watcher = std::make_shared<FileWatcher>();
    auto fs_mgr = std::make_shared<FilesystemManager>(mock_adapter, tx_mgr, watcher);
    auto term_exec = std::make_shared<TerminalExecutor>(mock_adapter);
    auto proj_mgr = std::make_shared<ProjectContextManager>();
    auto browser_mgr = std::make_shared<BrowserManager>();
    auto win_mgr = std::make_shared<WindowManager>(mock_adapter);
    auto input_mgr = std::make_shared<InputManager>(mock_adapter);
    auto clip_mgr = std::make_shared<ClipboardManager>(mock_adapter);
    auto screen_mgr = std::make_shared<ScreenCaptureManager>(mock_adapter);
    auto display_mgr = std::make_shared<DisplayManager>(mock_adapter);
    auto media_mgr = std::make_shared<MediaManager>(mock_adapter);
    auto state_prov = std::make_shared<SystemStateProvider>(mock_adapter);
    auto net_mgr = std::make_shared<NetworkManager>(mock_adapter);
    auto power_mgr = std::make_shared<PowerManager>(mock_adapter);
    auto notif_mgr = std::make_shared<NotificationManager>(mock_adapter);
    auto journal = std::make_shared<SystemActionJournal>();

    auto gateway = std::make_shared<ToolGateway>(
        policy, perm, bus, audit,
        app_mgr, proc_mgr, fs_mgr, term_exec, proj_mgr,
        browser_mgr, win_mgr, input_mgr, clip_mgr, screen_mgr,
        display_mgr, media_mgr, state_prov, net_mgr, power_mgr,
        notif_mgr, journal
    );

    const int iterations = 10000;

    // 1. Application Registry Resolution
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto opt = app_reg->resolve("vs code");
        (void)opt;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double app_res_ns = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()) / iterations;
    std::cout << "  • Application Registry Alias Resolution: " << app_res_ns << " ns/op\n";

    // 2. Policy Engine Evaluation
    PolicyContext p_ctx;
    p_ctx.actor_id = "user";
    p_ctx.capability_id = "system.application.open";
    t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto dec = policy->evaluate(p_ctx);
        (void)dec;
    }
    t1 = std::chrono::high_resolution_clock::now();
    double policy_ns = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()) / iterations;
    std::cout << "  • Policy Engine Evaluation:             " << policy_ns << " ns/op\n";

    // 3. Fast Path Dispatch Latency
    t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto res = gateway->execute_fast_path("set_volume", {{"level", "70"}});
        (void)res;
    }
    t1 = std::chrono::high_resolution_clock::now();
    double fast_path_us = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count()) / iterations;
    std::cout << "  • Fast Path Execution (10-Step Pipeline): " << fast_path_us << " µs/op\n";

    std::cout << "\n=======================================================\n";
    std::cout << "  Phase 4 Performance Benchmarks Complete\n";
    std::cout << "=======================================================\n";
    return 0;
}
