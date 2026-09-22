#pragma once

#include "../../contracts/tools/tool.hpp"
#include "../../contracts/common/result.hpp"
#include <chrono>
#include <string>

namespace vani::capabilities {

class SystemTimeExecutor : public contracts::Tool {
public:
    contracts::ToolManifest manifest() const override {
        return contracts::ToolManifest{
            .id = "system.time",
            .version = {1, 0, 0},
            .name = "System Time Capability",
            .description = "Returns current high-resolution epoch time in milliseconds",
            .input_schema_json = "{}",
            .output_schema_json = "{\"type\":\"object\",\"properties\":{\"epoch_ms\":{\"type\":\"integer\"}}}",
            .required_capabilities = {},
            .required_permissions = {},
            .risk_level = contracts::RiskLevel::Low,
            .default_timeout_ms = 1000,
            .is_reversible = false
        };
    }

    contracts::Result<contracts::ToolResult> execute(
        const contracts::ToolRequest& request
    ) override {
        if (request.cancellation_token.is_cancelled()) {
            return contracts::Fail(
                contracts::ErrorCode::Cancelled,
                "Execution cancelled before start",
                "system.time"
            );
        }

        const auto now_ms = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );

        return contracts::Ok(contracts::ToolResult{
            .tool_id = request.tool_id,
            .success = true,
            .output_json = "{\"epoch_ms\":" + std::to_string(now_ms) + "}",
            .execution_log = "Time retrieved successfully",
            .execution_duration_ms = 1
        });
    }
};

} // namespace vani::capabilities
