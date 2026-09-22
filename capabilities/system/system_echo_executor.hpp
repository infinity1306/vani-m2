#pragma once

#include "../../contracts/tools/tool.hpp"
#include "../../contracts/common/result.hpp"
#include <string>

namespace vani::capabilities {

class SystemEchoExecutor : public contracts::Tool {
public:
    contracts::ToolManifest manifest() const override {
        return contracts::ToolManifest{
            .id = "system.echo",
            .version = {1, 0, 0},
            .name = "System Echo Capability",
            .description = "Returns the provided input message verbatim",
            .input_schema_json = "{\"type\":\"object\",\"properties\":{\"message\":{\"type\":\"string\"}}}",
            .output_schema_json = "{\"type\":\"object\",\"properties\":{\"echo\":{\"type\":\"string\"}}}",
            .required_capabilities = {},
            .required_permissions = {},
            .risk_level = contracts::RiskLevel::Low,
            .default_timeout_ms = 1000,
            .is_reversible = true
        };
    }

    contracts::Result<contracts::ToolResult> execute(
        const contracts::ToolRequest& request
    ) override {
        if (request.cancellation_token.is_cancelled()) {
            return contracts::Fail(
                contracts::ErrorCode::Cancelled,
                "Execution cancelled before start",
                "system.echo"
            );
        }

        return contracts::Ok(contracts::ToolResult{
            .tool_id = request.tool_id,
            .success = true,
            .output_json = request.arguments_json,
            .execution_log = "Echo executed successfully",
            .execution_duration_ms = 1
        });
    }
};

} // namespace vani::capabilities
