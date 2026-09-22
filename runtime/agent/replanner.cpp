#include "replanner.hpp"
#include "agent_telemetry.hpp"

namespace vani::runtime::agent {

ReplanResult Replanner::handle_ambiguity(
    const contracts::AgentPlan& original_plan,
    const std::string& step_id,
    const std::vector<std::string>& candidates
) const {
    (void)original_plan;
    (void)step_id;
    ReplanResult res;
    res.success = false;
    res.is_ambiguous = true;
    res.ambiguous_candidates = candidates;
    res.error_message = "AMBIGUOUS_TARGET: Found multiple matching targets (" +
                        std::to_string(candidates.size()) + "). User clarification required.";
    return res;
}

ReplanResult Replanner::replan(
    const contracts::AgentPlan& original_plan,
    const std::string& failed_step_id,
    const contracts::AgentObservation& failure_obs,
    uint32_t current_replan_count,
    uint32_t max_replans
) const {
    ReplanResult res;

    // Check replan limit
    if (current_replan_count >= max_replans) {
        res.success = false;
        res.error_message = "PLAN_FAILED: Maximum replan limit reached (" +
                            std::to_string(max_replans) + ")";
        return res;
    }

    AgentTelemetryCollector::instance().record(
        AgentTelemetryPhase::A10_Replan, original_plan.request_id, original_plan.plan_id, failed_step_id,
        "Replan attempt " + std::to_string(current_replan_count + 1)
    );

    // Build replacement plan
    res.new_plan = original_plan;
    res.new_plan.plan_id = original_plan.plan_id + "_replan" + std::to_string(current_replan_count + 1);

    bool replaced = false;
    for (auto& s : res.new_plan.steps) {
        if (s.step_id == failed_step_id) {
            // Modify step to use safer fallback or alternate parameters
            if (s.capability_id == "application.launch") {
                // If specific target failed, fallback to generic or system default
                if (s.arguments.count("app_name") && s.arguments["app_name"] == "Visual Studio Code") {
                    s.arguments["app_name"] = "code";
                } else {
                    s.arguments["fallback"] = "true";
                }
                s.retry_policy_max_attempts = 1;
                replaced = true;
            } else if (s.capability_id == "browser.open_url") {
                // Try HTTPS fallback or clean URL
                s.arguments["clean_params"] = "true";
                replaced = true;
            } else {
                // Generic replacement parameter
                s.arguments["replan_mode"] = "recovery";
                replaced = true;
            }
        }
    }

    if (!replaced) {
        res.success = false;
        res.error_message = "Failed step not found in original plan: " + failed_step_id;
        return res;
    }

    res.success = true;
    return res;
}

} // namespace vani::runtime::agent
