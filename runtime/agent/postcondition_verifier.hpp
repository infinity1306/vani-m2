#pragma once

#include "../../contracts/agents/agent_execution_contracts.hpp"
#include "../../capabilities/system/gateway/tool_gateway.hpp"
#include <memory>
#include <string>

namespace vani::runtime::agent {

struct VerificationResult {
    bool verified{false};
    std::string evidence;
    std::string failure_reason;
};

class PostconditionVerifier {
public:
    explicit PostconditionVerifier(capabilities::system::ToolGatewayPtr gateway = nullptr);
    ~PostconditionVerifier() = default;

    [[nodiscard]] VerificationResult verify(
        const contracts::AgentStep& step,
        const contracts::ToolResult& tool_res
    ) const;

    void set_force_verification_failure(bool force) { force_failure_ = force; }

private:
    capabilities::system::ToolGatewayPtr gateway_;
    bool force_failure_{false};
};

using PostconditionVerifierPtr = std::shared_ptr<PostconditionVerifier>;

} // namespace vani::runtime::agent
