#pragma once

#include "../../contracts/agents/agent_execution_contracts.hpp"
#include "../../contracts/common/result.hpp"
#include <string>
#include <vector>
#include <unordered_set>

namespace vani::runtime::agent {

struct PlanValidationResult {
    bool is_valid{false};
    std::string error_message;
    std::vector<std::string> validation_errors;
};

class PlanValidator {
public:
    PlanValidator();
    ~PlanValidator() = default;

    [[nodiscard]] PlanValidationResult validate_plan(
        const contracts::AgentPlan& plan,
        const contracts::AgentResourceBudget& budget
    ) const;

    void register_known_capability(const std::string& capability_id);
    void register_known_tool(const std::string& tool_id);

private:
    [[nodiscard]] bool check_acyclic(
        const contracts::AgentPlan& plan,
        std::string& cycle_error
    ) const;

    [[nodiscard]] bool check_argument_safety(
        const contracts::AgentStep& step,
        std::string& safety_error
    ) const;

    std::unordered_set<std::string> known_capabilities_;
    std::unordered_set<std::string> known_tools_;
};

} // namespace vani::runtime::agent
