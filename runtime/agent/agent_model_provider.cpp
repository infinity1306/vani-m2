#include "agent_model_provider.hpp"
#include <chrono>
#include <windows.h>
#include <psapi.h>

namespace vani::runtime::agent {

// ============================================================================
// 1. DeterministicRulePlanner Implementation
// ============================================================================
contracts::Result<contracts::AgentPlan> DeterministicRulePlanner::generate_plan(
    const contracts::AgentRequest& request,
    const ShortTermTaskMemoryPtr& memory
) {
    (void)memory;
    contracts::AgentPlan plan;
    plan.plan_id = "plan_" + request.request_id;
    plan.request_id = request.request_id;
    plan.planner_provider = provider_id();
    plan.created_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    const std::string& goal = request.goal;

    std::string lower_goal = goal;
    for (char& c : lower_goal) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

    // Pattern 0: Specific Notepad Launch or Close
    if (lower_goal.find("notepad") != std::string::npos) {
        if (lower_goal.find("close") != std::string::npos ||
            lower_goal.find("kill") != std::string::npos ||
            lower_goal.find("stop") != std::string::npos ||
            lower_goal.find("band") != std::string::npos) {
            contracts::AgentStep s1;
            s1.step_id = "step_1";
            s1.capability_id = "application.close";
            s1.tool_id = "app_manager.close";
            s1.arguments["app_name"] = "notepad";
            s1.expected_postcondition = "process.stopped:notepad.exe";
            s1.risk_level = contracts::RiskLevel::Medium;
            s1.retry_policy_max_attempts = 2;
            s1.timeout_ms = 10000;

            plan.steps = {s1};
            plan.expected_outcomes = {"Notepad application closed"};
            return contracts::Result<contracts::AgentPlan>::success(plan);
        } else {
            contracts::AgentStep s1;
            s1.step_id = "step_1";
            s1.capability_id = "application.launch";
            s1.tool_id = "app_manager.launch";
            s1.arguments["app_name"] = "notepad";
            s1.expected_postcondition = "process.running:notepad.exe";
            s1.risk_level = contracts::RiskLevel::Low;
            s1.retry_policy_max_attempts = 2;
            s1.timeout_ms = 15000;

            plan.steps = {s1};
            plan.expected_outcomes = {"Notepad application launched"};
            return contracts::Result<contracts::AgentPlan>::success(plan);
        }
    }

    // Pattern 1: Launch application + open URL
    // e.g. "Chrome kholo aur Google open karo", "Open Chrome and visit github"
    if ((goal.find("Chrome") != std::string::npos || goal.find("chrome") != std::string::npos) &&
        (goal.find("Google") != std::string::npos || goal.find("google") != std::string::npos ||
         goal.find("open karo") != std::string::npos || goal.find("visit") != std::string::npos ||
         goal.find("aur") != std::string::npos)) {
        
        contracts::AgentStep s1;
        s1.step_id = "step_1";
        s1.capability_id = "application.launch";
        s1.tool_id = "app_manager.launch";
        s1.arguments["app_name"] = "Google Chrome";
        s1.expected_postcondition = "process.running:chrome.exe";
        s1.risk_level = contracts::RiskLevel::Low;
        s1.retry_policy_max_attempts = 2;
        s1.timeout_ms = 15000;

        contracts::AgentStep s2;
        s2.step_id = "step_2";
        s2.capability_id = "browser.open_url";
        s2.tool_id = "browser_manager.open";
        s2.arguments["url"] = (goal.find("github") != std::string::npos) ? "https://github.com" : "https://google.com";
        s2.dependencies.push_back("step_1");
        s2.expected_postcondition = "browser.navigated";
        s2.risk_level = contracts::RiskLevel::Low;
        s2.retry_policy_max_attempts = 2;
        s2.timeout_ms = 15000;

        plan.steps = {s1, s2};
        plan.dependencies = {{"step_1", "step_2"}};
        plan.expected_outcomes = {"Google Chrome launched", "Target URL opened in browser"};
        return contracts::Result<contracts::AgentPlan>::success(plan);
    }

    // Pattern 2: Multi-step project open and inspection
    if (goal.find("VS Code") != std::string::npos || (goal.find("code") != std::string::npos && goal.find("vani") != std::string::npos)) {
        contracts::AgentStep s1;
        s1.step_id = "step_1";
        s1.capability_id = "application.launch";
        s1.tool_id = "app_manager.launch";
        s1.arguments["app_name"] = "Visual Studio Code";
        s1.expected_postcondition = "process.running:code.exe";
        s1.risk_level = contracts::RiskLevel::Low;
        s1.retry_policy_max_attempts = 2;
        s1.timeout_ms = 15000;

        contracts::AgentStep s2;
        s2.step_id = "step_2";
        s2.capability_id = "project.inspect";
        s2.tool_id = "project_manager.inspect";
        s2.arguments["project_name"] = "vani mark 2";
        s2.dependencies.push_back("step_1");
        s2.expected_postcondition = "project.inspected";
        s2.risk_level = contracts::RiskLevel::Low;
        s2.retry_policy_max_attempts = 2;
        s2.timeout_ms = 15000;

        plan.steps = {s1, s2};
        plan.dependencies = {{"step_1", "step_2"}};
        plan.expected_outcomes = {"Visual Studio Code opened", "VANI project build state inspected"};
        return contracts::Result<contracts::AgentPlan>::success(plan);
    }

    // Pattern 3: Generic Open / Launch Application
    if (lower_goal.starts_with("open ") || lower_goal.starts_with("launch ") || lower_goal.starts_with("start ")) {
        size_t p = lower_goal.find(' ');
        std::string app_target = lower_goal.substr(p + 1);
        size_t sp = app_target.find_first_of(" \t\r\n");
        if (sp != std::string::npos) app_target = app_target.substr(0, sp);

        if (!app_target.empty()) {
            contracts::AgentStep s1;
            s1.step_id = "step_1";
            s1.capability_id = "application.launch";
            s1.tool_id = "app_manager.launch";
            s1.arguments["app_name"] = app_target;
            s1.expected_postcondition = "process.running:" + app_target;
            s1.risk_level = contracts::RiskLevel::Low;
            s1.retry_policy_max_attempts = 2;
            s1.timeout_ms = 15000;

            plan.steps = {s1};
            plan.expected_outcomes = {app_target + " application launched"};
            return contracts::Result<contracts::AgentPlan>::success(plan);
        }
    }

    // Pattern 4: Close / Stop Application
    if (lower_goal.starts_with("close ") || lower_goal.starts_with("kill ") || lower_goal.starts_with("stop ")) {
        size_t p = lower_goal.find(' ');
        std::string app_target = lower_goal.substr(p + 1);
        size_t sp = app_target.find_first_of(" \t\r\n");
        if (sp != std::string::npos) app_target = app_target.substr(0, sp);

        if (!app_target.empty()) {
            contracts::AgentStep s1;
            s1.step_id = "step_1";
            s1.capability_id = "application.close";
            s1.tool_id = "app_manager.close";
            s1.arguments["app_name"] = app_target;
            s1.expected_postcondition = "process.stopped:" + app_target;
            s1.risk_level = contracts::RiskLevel::Medium;
            s1.retry_policy_max_attempts = 2;
            s1.timeout_ms = 10000;

            plan.steps = {s1};
            plan.expected_outcomes = {app_target + " application closed"};
            return contracts::Result<contracts::AgentPlan>::success(plan);
        }
    }

    // Pattern 5: YouTube / Media Play
    if (lower_goal.starts_with("play ") || lower_goal.find("youtube") != std::string::npos) {
        std::string target = goal;
        if (lower_goal.starts_with("play ")) target = goal.substr(5);
        contracts::AgentStep s1;
        s1.step_id = "step_1";
        s1.capability_id = "media.youtube.play";
        s1.tool_id = "media.youtube.play";
        s1.arguments["target"] = target;
        s1.expected_postcondition = "";
        s1.risk_level = contracts::RiskLevel::Low;
        s1.retry_policy_max_attempts = 1;
        s1.timeout_ms = 15000;

        plan.steps = {s1};
        plan.expected_outcomes = {"Playing media: " + target};
        return contracts::Result<contracts::AgentPlan>::success(plan);
    }

    // Pattern 6: Web Search
    if (lower_goal.starts_with("search ") || lower_goal.starts_with("find ")) {
        size_t p = lower_goal.find(' ');
        std::string q = goal.substr(p + 1);
        contracts::AgentStep s1;
        s1.step_id = "step_1";
        s1.capability_id = "web.search";
        s1.tool_id = "web.search";
        s1.arguments["query"] = q;
        s1.expected_postcondition = "";
        s1.risk_level = contracts::RiskLevel::Low;
        s1.retry_policy_max_attempts = 1;
        s1.timeout_ms = 15000;

        plan.steps = {s1};
        plan.expected_outcomes = {"Search results for " + q};
        return contracts::Result<contracts::AgentPlan>::success(plan);
    }

    // Pattern 7: Weather
    if (lower_goal.find("weather") != std::string::npos) {
        std::string loc = "auto";
        size_t in_pos = lower_goal.find(" in ");
        if (in_pos != std::string::npos) {
            loc = goal.substr(in_pos + 4);
            size_t sp = loc.find_first_of(" ?.,\r\n");
            if (sp != std::string::npos) loc = loc.substr(0, sp);
        }
        contracts::AgentStep s1;
        s1.step_id = "step_1";
        s1.capability_id = "weather.get";
        s1.tool_id = "weather.get";
        s1.arguments["location"] = loc;
        s1.expected_postcondition = "";
        s1.risk_level = contracts::RiskLevel::Low;
        s1.retry_policy_max_attempts = 1;
        s1.timeout_ms = 10000;

        plan.steps = {s1};
        plan.expected_outcomes = {"Weather forecast fetched for " + loc};
        return contracts::Result<contracts::AgentPlan>::success(plan);
    }

    // Default multi-step fallback
    contracts::AgentStep s1;
    s1.step_id = "step_1";
    s1.capability_id = "system.status";
    s1.tool_id = "system_state.query";
    s1.expected_postcondition = "system.queried";
    s1.risk_level = contracts::RiskLevel::Low;
    s1.retry_policy_max_attempts = 1;
    s1.timeout_ms = 10000;

    plan.steps = {s1};
    plan.expected_outcomes = {"System state verified"};
    return contracts::Result<contracts::AgentPlan>::success(plan);
}

contracts::Result<contracts::AgentPlan> DeterministicRulePlanner::replan(
    const contracts::AgentRequest& request,
    const contracts::AgentPlan& current_plan,
    const std::string& failed_step_id,
    const contracts::AgentObservation& failure_obs,
    const ShortTermTaskMemoryPtr& memory
) {
    (void)request;
    (void)failure_obs;
    (void)memory;
    contracts::AgentPlan new_plan = current_plan;
    new_plan.plan_id = current_plan.plan_id + "_replan";

    for (auto& s : new_plan.steps) {
        if (s.step_id == failed_step_id) {
            s.arguments["retry_mode"] = "fallback";
            s.retry_policy_max_attempts = 1;
        }
    }
    return contracts::Result<contracts::AgentPlan>::success(new_plan);
}

contracts::Result<std::string> DeterministicRulePlanner::summarize_observation(
    const contracts::AgentObservation& observation
) {
    if (observation.success) {
        return contracts::Result<std::string>::success("Step " + observation.step_id + " succeeded: " + observation.evidence);
    } else {
        return contracts::Result<std::string>::success("Step " + observation.step_id + " failed: " + observation.error);
    }
}

// ============================================================================
// 2. LocalLLMPlanner Implementation
// ============================================================================
LocalLLMPlanner::LocalLLMPlanner(bool is_online, bool is_offline_mode)
    : is_online_(is_online), is_offline_mode_(is_offline_mode) {}

contracts::Result<contracts::AgentPlan> LocalLLMPlanner::generate_plan(
    const contracts::AgentRequest& request,
    const ShortTermTaskMemoryPtr& memory
) {
    if (!is_online_) {
        return contracts::Result<contracts::AgentPlan>::failure(
            contracts::ErrorCode::Unavailable,
            "AGENT_UNAVAILABLE_OFFLINE: Local LLM worker runtime is offline and no cloud fallback allowed."
        );
    }
    DeterministicRulePlanner fallback;
    return fallback.generate_plan(request, memory);
}

contracts::Result<contracts::AgentPlan> LocalLLMPlanner::replan(
    const contracts::AgentRequest& request,
    const contracts::AgentPlan& current_plan,
    const std::string& failed_step_id,
    const contracts::AgentObservation& failure_obs,
    const ShortTermTaskMemoryPtr& memory
) {
    if (!is_online_) {
        return contracts::Result<contracts::AgentPlan>::failure(
            contracts::ErrorCode::Unavailable,
            "AGENT_UNAVAILABLE_OFFLINE: Cannot replan while model is offline."
        );
    }
    DeterministicRulePlanner fallback;
    return fallback.replan(request, current_plan, failed_step_id, failure_obs, memory);
}

contracts::Result<std::string> LocalLLMPlanner::summarize_observation(
    const contracts::AgentObservation& observation
) {
    DeterministicRulePlanner fallback;
    return fallback.summarize_observation(observation);
}

// ============================================================================
// 3. MockOrTestPlanner Implementation
// ============================================================================
contracts::Result<contracts::AgentPlan> MockOrTestPlanner::generate_plan(
    const contracts::AgentRequest& request,
    const ShortTermTaskMemoryPtr& memory
) {
    (void)request;
    (void)memory;

    if (sim_resource_constrained_) {
        return contracts::Result<contracts::AgentPlan>::failure(
            contracts::ErrorCode::ResourceExhausted,
            "RESOURCE_CONSTRAINED: Insufficient memory/CPU budget for agent task execution."
        );
    }

    if (sim_offline_fail_) {
        return contracts::Result<contracts::AgentPlan>::failure(
            contracts::ErrorCode::Unavailable,
            "AGENT_UNAVAILABLE_OFFLINE: Cloud provider is offline and local model unavailable."
        );
    }

    if (!canned_plan_.steps.empty()) {
        return contracts::Result<contracts::AgentPlan>::success(canned_plan_);
    }

    DeterministicRulePlanner fallback;
    return fallback.generate_plan(request, memory);
}

contracts::Result<contracts::AgentPlan> MockOrTestPlanner::replan(
    const contracts::AgentRequest& request,
    const contracts::AgentPlan& current_plan,
    const std::string& failed_step_id,
    const contracts::AgentObservation& failure_obs,
    const ShortTermTaskMemoryPtr& memory
) {
    DeterministicRulePlanner fallback;
    return fallback.replan(request, current_plan, failed_step_id, failure_obs, memory);
}

contracts::Result<std::string> MockOrTestPlanner::summarize_observation(
    const contracts::AgentObservation& observation
) {
    DeterministicRulePlanner fallback;
    return fallback.summarize_observation(observation);
}

// ============================================================================
// 4. ResourceGovernor Win32 Implementations
// ============================================================================
uint64_t ResourceGovernor::get_real_available_ram_mb() {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex)) {
        return statex.ullAvailPhys / (1024 * 1024);
    }
    return 0;
}

uint64_t ResourceGovernor::get_real_total_ram_mb() {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex)) {
        return statex.ullTotalPhys / (1024 * 1024);
    }
    return 0;
}

uint64_t ResourceGovernor::get_real_process_memory_mb() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / (1024 * 1024);
    }
    return 0;
}

} // namespace vani::runtime::agent
