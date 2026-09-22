#include "command_policy_evaluator.hpp"
#include <algorithm>
#include <sstream>

namespace vani::capabilities::system {

std::string_view CommandPolicyEvaluator::to_string(contracts::CommandRiskCategory risk) noexcept {
    switch (risk) {
        case contracts::CommandRiskCategory::Safe: return "SAFE";
        case contracts::CommandRiskCategory::Low: return "LOW";
        case contracts::CommandRiskCategory::Medium: return "MEDIUM";
        case contracts::CommandRiskCategory::High: return "HIGH";
        case contracts::CommandRiskCategory::Critical: return "CRITICAL";
        default: return "CRITICAL";
    }
}

contracts::CommandRiskCategory CommandPolicyEvaluator::classify_command(std::string_view command) {
    if (command.empty()) {
        return contracts::CommandRiskCategory::Safe;
    }

    std::string lower;
    lower.reserve(command.size());
    for (char c : command) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    // 1. Critical risk patterns (Disk wipe, shutdown, privilege escalation, shell piping to bash)
    static const std::vector<std::string> critical_patterns = {
        "format ", "mkfs", "dd if=", "shutdown", "reboot", "poweroff",
        ":(){ :|:& };:", "rm -rf /", "rm -rf /*", "del /f /s /q c:\\",
        "sudo su", "chmod -r 777 /", "curl | bash", "curl | sh", "wget | sh",
        "curl | powershell", "iex (new-object net.webclient)"
    };

    for (const auto& pat : critical_patterns) {
        if (lower.find(pat) != std::string::npos) {
            return contracts::CommandRiskCategory::Critical;
        }
    }

    // 2. High risk patterns (Deletion, permission change, killing processes, network config)
    static const std::vector<std::string> high_patterns = {
        "rm ", "rmdir", "del ", "erase ", "chmod ", "chown ", "kill ",
        "taskkill", "iptables", "netsh", "useradd", "net user", "npm publish",
        "git push --force", "git reset --hard"
    };

    for (const auto& pat : high_patterns) {
        if (lower.find(pat) != std::string::npos) {
            return contracts::CommandRiskCategory::High;
        }
    }

    // 3. Medium risk patterns (Build, test, install, compile)
    static const std::vector<std::string> medium_patterns = {
        "npm install", "npm run", "yarn add", "cargo build", "cargo test",
        "cmake", "make", "pytest", "pip install", "mvn", "gradle", "dotnet build"
    };

    for (const auto& pat : medium_patterns) {
        if (lower.find(pat) != std::string::npos) {
            return contracts::CommandRiskCategory::Medium;
        }
    }

    // 4. Low risk patterns (Inspect, diff, display)
    static const std::vector<std::string> low_patterns = {
        "git diff", "git log", "echo ", "cat ", "type ", "head ", "tail ",
        "grep ", "findstr", "whoami", "hostname", "uname"
    };

    for (const auto& pat : low_patterns) {
        if (lower.find(pat) != std::string::npos) {
            return contracts::CommandRiskCategory::Low;
        }
    }

    // 5. Safe read-only commands
    static const std::vector<std::string> safe_patterns = {
        "ls", "dir", "pwd", "cd", "git status", "git branch", "date", "time"
    };

    for (const auto& pat : safe_patterns) {
        if (lower == pat || lower.rfind(pat + " ", 0) == 0) {
            return contracts::CommandRiskCategory::Safe;
        }
    }

    // Default to Medium if unrecognized
    return contracts::CommandRiskCategory::Medium;
}

bool CommandPolicyEvaluator::is_command_allowed_in_mode(
    std::string_view command,
    contracts::TerminalExecutionMode mode
) {
    auto risk = classify_command(command);
    switch (mode) {
        case contracts::TerminalExecutionMode::Normal:
            return risk != contracts::CommandRiskCategory::Critical;
        case contracts::TerminalExecutionMode::Restricted:
            return risk == contracts::CommandRiskCategory::Safe ||
                   risk == contracts::CommandRiskCategory::Low ||
                   risk == contracts::CommandRiskCategory::Medium;
        case contracts::TerminalExecutionMode::Sandboxed:
            return risk == contracts::CommandRiskCategory::Safe ||
                   risk == contracts::CommandRiskCategory::Low;
    }
    return false;
}

} // namespace vani::capabilities::system
