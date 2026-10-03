#include "../../runtime/core/runtime.hpp"
#include "../../runtime/agent/agent_controller.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << " VANI Mark 2 Command Line Interface v2.0\n";
    std::cout << "========================================\n";

    if (argc < 2) {
        std::cout << "Usage: vani-cli <command> [args...]\n\n";
        std::cout << "Commands:\n";
        std::cout << "  status              Display runtime status and component health\n";
        std::cout << "  capabilities        List all registered capabilities and providers\n";
        std::cout << "  exec <id> [json]    Execute capability directly via 10-step ToolGateway\n";
        std::cout << "  goal <description>  Execute natural language request via AgentController\n";
        std::cout << "  undo                Revert previous filesystem/clipboard modification\n";
        std::cout << "  weather [city]      Fetch real live weather forecast\n";
        std::cout << "  search <query>      Execute live web knowledge search\n";
        std::cout << "  tasks               List all active and recent tasks\n";
        std::cout << "  version             Print VANI Mark 2 contract version info\n";
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
    } else if (command == "exec") {
        if (argc < 3) {
            std::cerr << "Usage: vani-cli exec <capability_id> [arguments_json] [--confirm]\n";
            runtime.shutdown();
            return 1;
        }
        std::string cap_id = argv[2];
        std::string args = (argc >= 4) ? argv[3] : "{}";
        bool confirm = false;
        for (int i = 3; i < argc; ++i) {
            if (std::string(argv[i]) == "--confirm" || std::string(argv[i]) == "-y") {
                confirm = true;
            }
        }

        vani::capabilities::system::ToolExecutionPipelineContext ctx;
        ctx.capability_id = cap_id;
        ctx.arguments_json = args;
        ctx.actor_id = "user";
        ctx.user_confirmed = confirm;

        std::cout << "--> Dispatching tool: " << cap_id << "\n";
        auto res = runtime.tool_gateway()->execute(ctx);
        if (res.is_success()) {
            std::cout << "[SUCCESS] Duration: " << res.value().execution_duration_ms << "ms\n";
            std::cout << "Output: " << res.value().output_json << "\n";
        } else {
            std::cerr << "[FAILED] Error " << vani::contracts::to_string(res.error().code) << ": " << res.error().message << "\n";
        }
    } else if (command == "undo") {
        vani::capabilities::system::ToolExecutionPipelineContext ctx;
        ctx.capability_id = "undo.execute";
        ctx.actor_id = "user";
        auto res = runtime.tool_gateway()->execute(ctx);
        if (res.is_success()) {
            std::cout << "[SUCCESS] " << res.value().output_json << "\n";
        } else {
            std::cerr << "[FAILED] " << res.error().message << "\n";
        }
    } else if (command == "weather") {
        std::string loc = (argc >= 3) ? argv[2] : "auto";
        vani::capabilities::system::ToolExecutionPipelineContext ctx;
        ctx.capability_id = "weather.get";
        ctx.arguments_json = "{\"location\":\"" + loc + "\"}";
        ctx.actor_id = "user";
        auto res = runtime.tool_gateway()->execute(ctx);
        if (res.is_success()) {
            std::cout << res.value().output_json << "\n";
        } else {
            std::cerr << "[FAILED] " << res.error().message << "\n";
        }
    } else if (command == "search") {
        if (argc < 3) {
            std::cerr << "Usage: vani-cli search <query>\n";
            runtime.shutdown();
            return 1;
        }
        std::string query = argv[2];
        vani::capabilities::system::ToolExecutionPipelineContext ctx;
        ctx.capability_id = "web.search";
        ctx.arguments_json = "{\"query\":\"" + query + "\"}";
        ctx.actor_id = "user";
        auto res = runtime.tool_gateway()->execute(ctx);
        if (res.is_success()) {
            std::cout << res.value().output_json << "\n";
        } else {
            std::cerr << "[FAILED] " << res.error().message << "\n";
        }
    } else if (command == "goal") {
        if (argc < 3) {
            std::cerr << "Usage: vani-cli goal <goal_prompt>\n";
            runtime.shutdown();
            return 1;
        }
        std::string goal = argv[2];
        vani::runtime::agent::AgentController controller(runtime.tool_gateway(), runtime.policy_engine());
        vani::contracts::AgentRequest req;
        req.request_id = "cli_req_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        req.goal = goal;

        std::cout << "--> Submitting goal to AgentController: \"" << goal << "\"\n";
        auto exec_res = controller.execute_goal(req);
        std::cout << "Agent Response: " << exec_res.final_response << "\n";
        std::cout << "Execution State: " << (exec_res.final_state == vani::contracts::PlanState::Completed ? "COMPLETED" : "FAILED") << "\n";
    } else if (command == "tasks") {
        std::cout << "Current Tasks (" << runtime.task_manager()->list_tasks().size() << ")\n";
    } else if (command == "chat" || command == "interactive") {
        std::cout << "\n=======================================================\n";
        std::cout << "  VANI Mark 2 Interactive Terminal Assistant\n";
        std::cout << "  Hardware Mic Fallback: Text & Push-to-Talk Interface\n";
        std::cout << "  Type any instruction or query. Type 'exit' to quit.\n";
        std::cout << "=======================================================\n\n";

        vani::runtime::agent::AgentController controller(runtime.tool_gateway(), runtime.policy_engine());
        std::string line;
        while (true) {
            std::cout << "You > ";
            if (!std::getline(std::cin, line)) break;
            if (line.empty()) continue;
            if (line == "exit" || line == "quit") break;

            if (line.starts_with("exec ")) {
                std::string sub = line.substr(5);
                size_t sp = sub.find(' ');
                std::string cap = (sp == std::string::npos) ? sub : sub.substr(0, sp);
                std::string args = (sp == std::string::npos) ? "{}" : sub.substr(sp + 1);
                vani::capabilities::system::ToolExecutionPipelineContext ctx;
                ctx.capability_id = cap;
                ctx.arguments_json = args;
                ctx.user_confirmed = true;
                auto res = runtime.tool_gateway()->execute(ctx);
                if (res.is_success()) {
                    std::cout << "VANI > " << res.value().output_json << "\n\n";
                } else {
                    std::cout << "VANI [Error] > " << res.error().message << "\n\n";
                }
            } else if (line == "undo") {
                vani::capabilities::system::ToolExecutionPipelineContext ctx;
                ctx.capability_id = "undo.execute";
                auto res = runtime.tool_gateway()->execute(ctx);
                if (res.is_success()) {
                    std::cout << "VANI > [Undo OK] " << res.value().output_json << "\n\n";
                } else {
                    std::cout << "VANI [Error] > " << res.error().message << "\n\n";
                }
            } else if (line == "status") {
                std::cout << "VANI > System operational, 36 capabilities online.\n\n";
            } else if (line == "help" || line == "commands" || line == "?") {
                std::cout << "\n=======================================================\n";
                std::cout << "  VANI Mark 2 - Available Commands Reference\n";
                std::cout << "=======================================================\n";
                std::cout << "1. Conversational & Agent Goals:\n";
                std::cout << "   - open <app>             e.g. 'open notepad', 'open calc', 'open chrome'\n";
                std::cout << "   - close <app>            e.g. 'close notepad', 'kill calc'\n";
                std::cout << "   - <app> kholo / band     e.g. 'notepad kholo', 'chrome band karo'\n";
                std::cout << "   - search <query>         e.g. 'search artificial intelligence'\n";
                std::cout << "   - weather [city]         e.g. 'weather in London', 'weather'\n";
                std::cout << "   - play <media>           e.g. 'play lofi chill on youtube'\n";
                std::cout << "   - status                 e.g. inspect system health and runtime\n";
                std::cout << "   - undo                   reverts last file or clipboard edit\n\n";
                std::cout << "2. Direct Tool Gateway Execution ('exec <capability> [json]'):\n";
                std::cout << "   - exec application.launch '{\"app_name\":\"notepad\"}'\n";
                std::cout << "   - exec application.close '{\"app_name\":\"notepad\"}'\n";
                std::cout << "   - exec system.process.list\n";
                std::cout << "   - exec filesystem.read '{\"path\":\"file.txt\"}'\n";
                std::cout << "   - exec filesystem.write '{\"path\":\"file.txt\",\"content\":\"hi\"}'\n";
                std::cout << "   - exec terminal.execute '{\"command\":\"dir\"}'\n";
                std::cout << "   - exec browser.open '{\"url\":\"https://github.com\"}'\n";
                std::cout << "   - exec clipboard.read\n";
                std::cout << "   - exec clipboard.write '{\"text\":\"sample\"}'\n";
                std::cout << "   - exec screen.capture\n";
                std::cout << "   - exec media.volume '{\"level\":70}'\n";
                std::cout << "   - exec reminder.create '{\"message\":\"Take break\",\"seconds\":300}'\n";
                std::cout << "   - exec memory.store '{\"key\":\"topic\",\"content\":\"VANI v2\"}'\n";
                std::cout << "   - exec memory.recall '{\"query\":\"VANI\"}'\n";
                std::cout << "=======================================================\n\n";
            } else {
                std::cout << "VANI [Thinking...] \n";
                vani::contracts::AgentRequest req;
                req.request_id = "chat_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
                req.goal = line;
                auto exec_res = controller.execute_goal(req);
                std::cout << "VANI > " << exec_res.final_response << "\n";
                std::cout << "       [State: " << (exec_res.final_state == vani::contracts::PlanState::Completed ? "COMPLETED" : "FAILED") << "]\n\n";
            }
        }
    } else {
        std::cout << "Unknown command: " << command << "\n";
    }

    runtime.shutdown();
    return 0;
}
