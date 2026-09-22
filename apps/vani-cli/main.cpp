#include "../../runtime/core/runtime.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << " VANI Mark 2 Command Line Interface v2.0\n";
    std::cout << "========================================\n";

    if (argc < 2) {
        std::cout << "Usage: vani-cli <command> [args...]\n\n";
        std::cout << "Commands:\n";
        std::cout << "  status       Display runtime status and component health\n";
        std::cout << "  capabilities List all registered capabilities and providers\n";
        std::cout << "  tasks        List all active and recent tasks\n";
        std::cout << "  version      Print VANI Mark 2 contract version info\n";
        return 0;
    }

    std::string command = argv[1];
    if (command == "version") {
        std::cout << "VANI Mark 2.0.0 (Contract ABI v1.0.0)\n";
        std::cout << "Local-First AI Operating Layer Architecture\n";
        return 0;
    }

    vani::runtime::VaniRuntime runtime;
    runtime.initialize();
    runtime.start();

    if (command == "status") {
        std::cout << "Runtime State: " << vani::runtime::to_string(runtime.state()) << "\n";
        std::cout << "Subsystem Health:\n";
        for (const auto& [name, h] : runtime.health_service()->get_all_health()) {
            std::cout << "  - " << name << ": [" << vani::observability::to_string(h.status) << "] " << h.message << "\n";
        }
    } else if (command == "capabilities") {
        std::cout << "Registered Capabilities (" << runtime.capability_registry()->count() << "):\n";
        for (const auto& cap : runtime.capability_registry()->list_all()) {
            std::cout << "  - " << cap.id << " -> " << cap.provider_id << " (Risk: " << vani::contracts::to_string(cap.risk_level) << ")\n";
        }
    } else if (command == "tasks") {
        std::cout << "Current Tasks (" << runtime.task_manager()->list_tasks().size() << ")\n";
    } else {
        std::cout << "Unknown command: " << command << "\n";
    }

    runtime.shutdown();
    return 0;
}
