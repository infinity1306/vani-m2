#include "terminal_executor.hpp"

namespace vani::capabilities::system {

TerminalExecutor::TerminalExecutor(adapters::system::SystemAdapterPtr adapter)
    : adapter_(std::move(adapter)) {}

contracts::Result<contracts::TerminalExecutionResult> TerminalExecutor::execute(
    const contracts::TerminalExecutionRequest& request,
    const contracts::CancellationToken& cancel_token
) {
    if (!adapter_) {
        return contracts::Result<contracts::TerminalExecutionResult>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }

    auto mode = (request.execution_mode == contracts::TerminalExecutionMode::Normal)
                    ? default_mode_
                    : request.execution_mode;

    if (!CommandPolicyEvaluator::is_command_allowed_in_mode(request.command, mode)) {
        return contracts::Result<contracts::TerminalExecutionResult>::failure(
            contracts::ErrorCode::SecurityViolation,
            "Command disallowed under " + std::string(mode == contracts::TerminalExecutionMode::Sandboxed ? "Sandboxed" : "Restricted") + " execution mode: " + request.command
        );
    }

    auto modified_req = request;
    if (modified_req.working_directory.empty()) {
        modified_req.working_directory = WorkingDirectoryResolver::resolve_cwd("", "", "", "", "/workspace");
    }

    auto res = adapter_->execute_command(modified_req, cancel_token);
    if (!res.is_success()) {
        return res;
    }

    auto result = res.value();
    // Enforce max output bytes
    if (result.stdout_content.size() > request.max_output_bytes) {
        result.stdout_content.resize(request.max_output_bytes);
        result.stdout_content += "\n[TRUNCATED: Max output bytes exceeded]";
        result.output_truncated = true;
    }

    return contracts::Result<contracts::TerminalExecutionResult>::success(result);
}

} // namespace vani::capabilities::system
