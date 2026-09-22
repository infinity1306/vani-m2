#pragma once

#include "command_policy_evaluator.hpp"
#include "working_directory_resolver.hpp"
#include "../../../contracts/capabilities/capability_descriptor.hpp"
#include "../../../contracts/tools/risk_level.hpp"
#include "../../../contracts/system/system_contracts.hpp"
#include "../../../adapters/system/system_adapter.hpp"
#include "../../../contracts/common/result.hpp"
#include "../../../contracts/common/cancellation_token.hpp"
#include <memory>
#include <string>

namespace vani::capabilities::system {

class TerminalExecutor {
public:
    explicit TerminalExecutor(adapters::system::SystemAdapterPtr adapter);
    ~TerminalExecutor() = default;

    contracts::Result<contracts::TerminalExecutionResult> execute(
        const contracts::TerminalExecutionRequest& request,
        const contracts::CancellationToken& cancel_token = contracts::CancellationToken::none()
    );

    void set_default_mode(contracts::TerminalExecutionMode mode) noexcept { default_mode_ = mode; }
    [[nodiscard]] contracts::TerminalExecutionMode default_mode() const noexcept { return default_mode_; }

private:
    adapters::system::SystemAdapterPtr adapter_;
    contracts::TerminalExecutionMode default_mode_{contracts::TerminalExecutionMode::Normal};
};

using TerminalExecutorPtr = std::shared_ptr<TerminalExecutor>;

} // namespace vani::capabilities::system
