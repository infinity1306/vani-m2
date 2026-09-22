#include "plan_validator.hpp"
#include <unordered_map>
#include <queue>
#include <algorithm>

namespace vani::runtime::agent {

PlanValidator::PlanValidator() {
    // Default known capabilities
    known_capabilities_ = {
        "application.launch",
        "application.close",
        "application.focus",
        "browser.open_url",
        "browser.navigate",
        "media.volume",
        "system.status",
        "system.power",
        "filesystem.read",
        "filesystem.write",
        "filesystem.list",
        "project.locate",
        "project.inspect",
        "terminal.execute"
    };

    known_tools_ = {
        "app_manager.launch",
        "app_manager.close",
        "browser_manager.open",
        "media_manager.set_volume",
        "system_state.query",
        "fs_manager.read",
        "fs_manager.write",
        "filesystem.read",
        "filesystem.write",
        "project_manager.inspect",
        "terminal.run_command"
    };
}

void PlanValidator::register_known_capability(const std::string& capability_id) {
    known_capabilities_.insert(capability_id);
}

void PlanValidator::register_known_tool(const std::string& tool_id) {
    known_tools_.insert(tool_id);
}

bool PlanValidator::check_argument_safety(const contracts::AgentStep& step, std::string& safety_error) const {
    // Check for malicious command injection patterns in argument keys/values
    static const std::vector<std::string> dangerous_patterns = {
        ";", "&&", "||", "|", "`", "$(", "${",
        "rm -rf", "format ", "powershell -enc",
        "cmd.exe /c del", "sudo ", ">nul", "/etc/passwd",
        ":(){ :|:& };:"
    };

    for (const auto& [k, v] : step.arguments) {
        // Prevent attempts to override policy or privilege elevation in arguments
        if (k == "override_policy" || k == "elevate_privilege" || k == "bypass_confirmation") {
            safety_error = "Adversarial argument detected: " + k;
            return false;
        }

        for (const auto& pat : dangerous_patterns) {
            if (v.find(pat) != std::string::npos) {
                safety_error = "Disallowed command injection pattern detected in argument '" + k + "': " + pat;
                return false;
            }
        }
    }
    return true;
}

bool PlanValidator::check_acyclic(const contracts::AgentPlan& plan, std::string& cycle_error) const {
    // Build adjacency list and in-degrees
    std::unordered_map<std::string, std::vector<std::string>> adj;
    std::unordered_map<std::string, int> in_degree;

    for (const auto& s : plan.steps) {
        in_degree[s.step_id] = 0;
    }

    // Add dependencies declared in steps
    for (const auto& s : plan.steps) {
        for (const auto& dep : s.dependencies) {
            if (in_degree.find(dep) == in_degree.end()) {
                cycle_error = "Step '" + s.step_id + "' depends on unknown step '" + dep + "'";
                return false;
            }
            adj[dep].push_back(s.step_id);
            in_degree[s.step_id]++;
        }
    }

    // Add dependencies declared at plan level
    for (const auto& [from, to] : plan.dependencies) {
        if (in_degree.find(from) == in_degree.end() || in_degree.find(to) == in_degree.end()) {
            cycle_error = "Plan dependency references unknown step: " + from + " -> " + to;
            return false;
        }
        adj[from].push_back(to);
        in_degree[to]++;
    }

    // Kahn's algorithm for cycle detection
    std::queue<std::string> q;
    for (const auto& [id, deg] : in_degree) {
        if (deg == 0) q.push(id);
    }

    size_t visited = 0;
    while (!q.empty()) {
        auto u = q.front();
        q.pop();
        visited++;

        for (const auto& v : adj[u]) {
            if (--in_degree[v] == 0) {
                q.push(v);
            }
        }
    }

    if (visited < in_degree.size()) {
        cycle_error = "Cyclic dependency graph detected in plan!";
        return false;
    }

    return true;
}

PlanValidationResult PlanValidator::validate_plan(
    const contracts::AgentPlan& plan,
    const contracts::AgentResourceBudget& budget
) const {
    PlanValidationResult res;
    res.is_valid = true;

    if (plan.steps.empty()) {
        res.is_valid = false;
        res.error_message = "Plan contains 0 steps";
        res.validation_errors.push_back(res.error_message);
        return res;
    }

    // Budget check: Max steps
    if (plan.steps.size() > budget.max_steps) {
        res.is_valid = false;
        res.error_message = "Plan step count (" + std::to_string(plan.steps.size()) +
                            ") exceeds maximum budget (" + std::to_string(budget.max_steps) + ")";
        res.validation_errors.push_back(res.error_message);
        return res;
    }

    // Step-by-step validations
    std::unordered_set<std::string> seen_step_ids;
    for (const auto& step : plan.steps) {
        if (step.step_id.empty()) {
            res.is_valid = false;
            res.validation_errors.push_back("Step ID cannot be empty");
        }

        if (seen_step_ids.count(step.step_id)) {
            res.is_valid = false;
            res.validation_errors.push_back("Duplicate step ID: " + step.step_id);
        }
        seen_step_ids.insert(step.step_id);

        // Capability validation
        if (known_capabilities_.find(step.capability_id) == known_capabilities_.end()) {
            res.is_valid = false;
            res.validation_errors.push_back("Unknown or unauthorized capability: " + step.capability_id);
        }

        // Tool validation
        if (!step.tool_id.empty() && known_tools_.find(step.tool_id) == known_tools_.end()) {
            res.is_valid = false;
            res.validation_errors.push_back("Unknown or unauthorized tool: " + step.tool_id);
        }

        // Retry policy bounded
        if (step.retry_policy_max_attempts > budget.max_retries_per_step) {
            res.is_valid = false;
            res.validation_errors.push_back("Step '" + step.step_id + "' retry policy (" +
                                            std::to_string(step.retry_policy_max_attempts) +
                                            ") exceeds allowed budget (" +
                                            std::to_string(budget.max_retries_per_step) + ")");
        }

        // Timeout bounded
        if (step.timeout_ms == 0 || step.timeout_ms > budget.global_timeout_ms) {
            res.is_valid = false;
            res.validation_errors.push_back("Step '" + step.step_id + "' timeout (" +
                                            std::to_string(step.timeout_ms) + " ms) is invalid or exceeds global budget");
        }

        // Argument safety / injection check
        std::string arg_err;
        if (!check_argument_safety(step, arg_err)) {
            res.is_valid = false;
            res.validation_errors.push_back(arg_err);
        }

        // State-modifying actions must specify expected postconditions
        if ((step.capability_id == "application.launch" ||
             step.capability_id == "application.close" ||
             step.capability_id == "filesystem.write" ||
             step.capability_id == "media.volume") &&
            step.expected_postcondition.empty()) {
            res.is_valid = false;
            res.validation_errors.push_back("State-modifying step '" + step.step_id +
                                            "' (" + step.capability_id + ") must specify an expected postcondition");
        }
    }

    // Acyclic check
    std::string cycle_err;
    if (!check_acyclic(plan, cycle_err)) {
        res.is_valid = false;
        res.validation_errors.push_back(cycle_err);
    }

    if (!res.is_valid && res.error_message.empty()) {
        res.error_message = res.validation_errors.front();
    }

    return res;
}

} // namespace vani::runtime::agent
