#include "../../runtime/core/runtime.hpp"
#include "../../observability/logger.hpp"
#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>

static std::atomic<bool> g_shutdown_requested{false};

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        g_shutdown_requested.store(true);
    }
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    vani::observability::Logger::instance().info(
        "vani-runtime",
        "Starting VANI Mark 2 Production Daemon (Phase 1)..."
    );

    vani::config::RuntimeProfile profile;
    vani::runtime::VaniRuntime runtime(profile);

    // Initialize
    auto init_res = runtime.initialize();
    if (init_res.is_err()) {
        std::cerr << "Initialization failed: " << init_res.error().message << "\n";
        return 1;
    }

    // Register baseline capabilities
    auto reg_res = runtime.capability_registry()->register_capability({
        .id = "coding.refactor",
        .version = {1, 0, 0},
        .type = vani::runtime::CapabilityType::Agent,
        .provider_id = "agent.odysseus",
        .description = "Autonomous AST refactoring capability",
        .required_permissions = {"filesystem.write", "terminal.execute"},
        .risk_level = vani::contracts::RiskLevel::Medium
    });

    if (reg_res.is_err()) {
        std::cerr << "Failed to register baseline capability: " << reg_res.error().message << "\n";
        return 1;
    }

    // Start
    auto start_res = runtime.start();
    if (start_res.is_err()) {
        std::cerr << "Runtime start failed: " << start_res.error().message << "\n";
        return 1;
    }

    vani::observability::Logger::instance().info(
        "vani-runtime",
        "VANI Core is operational. Waiting for tasks or IPC requests (Ctrl+C to terminate)..."
    );

    // Main execution loop
    int loops = 0;
    while (!g_shutdown_requested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        loops++;
        if (argc > 1 && std::string(argv[1]) == "--once" && loops > 5) {
            break; // Used for smoke testing
        }
    }

    // Graceful Shutdown
    vani::observability::Logger::instance().info(
        "vani-runtime",
        "Shutting down VANI Core gracefully..."
    );
    runtime.shutdown();

    return 0;
}
