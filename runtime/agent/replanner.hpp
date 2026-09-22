#pragma once

#include "../../contracts/agents/agent_execution_contracts.hpp"
#include <string>
#include <vector>

namespace vani::runtime::agent {

struct ReplanResult {
    bool success{false};
    contracts::AgentPlan new_plan;
    bool is_ambiguous{false};
    std::vector<std::string> ambiguous_candidates;
    std::string error_message;
};

class Replanner {
public:
    Replanner() = default;
    ~Replanner() = default;

    [[nodiscard]] ReplanResult replan(
        const contracts::AgentPlan& original_plan,
        const std::string& failed_step_id,
        const contracts::AgentObservation& failure_obs,
        uint32_t current_replan_count,
        uint32_t max_replans
    ) const;

    [[nodiscard]] ReplanResult handle_ambiguity(
        const contracts::AgentPlan& original_plan,
        const std::string& step_id,
        const std::vector<std::string>& candidates
    ) const;
};

} // namespace vani::runtime::agent
