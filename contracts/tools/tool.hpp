#pragma once

#include "tool_manifest.hpp"
#include "risk_level.hpp"
#include "common/version.hpp"
#include "common/result.hpp"
#include "common/cancellation_token.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace vani::contracts {

struct ToolRequest {
    ToolId tool_id;
    TaskId task_id;
    CorrelationId correlation_id;
    std::string arguments_json;
    std::unordered_map<std::string, std::string> execution_context;
    CancellationToken cancellation_token{CancellationToken::none()};
};

struct ToolResult {
    ToolId tool_id;
    bool success{false};
    std::string output_json;
    std::string execution_log;
    uint32_t execution_duration_ms{0};
};

class Tool {
public:
    virtual ~Tool() = default;

    [[nodiscard]] virtual ToolManifest manifest() const = 0;

    virtual Result<ToolResult> execute(const ToolRequest& request) = 0;
};

using ToolPtr = std::shared_ptr<Tool>;

} // namespace vani::contracts
