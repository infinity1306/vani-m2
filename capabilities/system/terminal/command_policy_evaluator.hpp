#pragma once

#include "../../../contracts/system/system_contracts.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace vani::capabilities::system {

class CommandPolicyEvaluator {
public:
    CommandPolicyEvaluator() = default;
    ~CommandPolicyEvaluator() = default;

    [[nodiscard]] static contracts::CommandRiskCategory classify_command(std::string_view command);
    [[nodiscard]] static bool is_command_allowed_in_mode(
        std::string_view command,
        contracts::TerminalExecutionMode mode
    );
    [[nodiscard]] static std::string_view to_string(contracts::CommandRiskCategory risk) noexcept;
};

} // namespace vani::capabilities::system
