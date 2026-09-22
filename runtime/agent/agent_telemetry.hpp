#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <vector>
#include <mutex>
#include <string_view>

namespace vani::runtime::agent {

enum class AgentTelemetryPhase : uint8_t {
    A0_AgentRequest = 0,
    A1_PlanningStart = 1,
    A2_PlanningComplete = 2,
    A3_ValidationComplete = 3,
    A4_PolicyCheck = 4,
    A5_StepStart = 5,
    A6_ToolRequest = 6,
    A7_ToolComplete = 7,
    A8_Verification = 8,
    A9_Observation = 9,
    A10_Replan = 10,
    A11_TaskComplete = 11,
    A12_TaskFailed = 12,
    A13_Cancellation = 13
};

[[nodiscard]] constexpr std::string_view to_string(AgentTelemetryPhase phase) noexcept {
    switch (phase) {
        case AgentTelemetryPhase::A0_AgentRequest: return "A0_AGENT_REQUEST";
        case AgentTelemetryPhase::A1_PlanningStart: return "A1_PLANNING_START";
        case AgentTelemetryPhase::A2_PlanningComplete: return "A2_PLANNING_COMPLETE";
        case AgentTelemetryPhase::A3_ValidationComplete: return "A3_VALIDATION_COMPLETE";
        case AgentTelemetryPhase::A4_PolicyCheck: return "A4_POLICY_CHECK";
        case AgentTelemetryPhase::A5_StepStart: return "A5_STEP_START";
        case AgentTelemetryPhase::A6_ToolRequest: return "A6_TOOL_REQUEST";
        case AgentTelemetryPhase::A7_ToolComplete: return "A7_TOOL_COMPLETE";
        case AgentTelemetryPhase::A8_Verification: return "A8_VERIFICATION";
        case AgentTelemetryPhase::A9_Observation: return "A9_OBSERVATION";
        case AgentTelemetryPhase::A10_Replan: return "A10_REPLAN";
        case AgentTelemetryPhase::A11_TaskComplete: return "A11_TASK_COMPLETE";
        case AgentTelemetryPhase::A12_TaskFailed: return "A12_TASK_FAILED";
        case AgentTelemetryPhase::A13_Cancellation: return "A13_CANCELLATION";
        default: return "UNKNOWN";
    }
}

struct AgentTelemetryRecord {
    AgentTelemetryPhase phase;
    uint64_t timestamp_ns;
    std::string task_id;
    std::string plan_id;
    std::string step_id;
    std::string details;
};

class AgentTelemetryCollector {
public:
    static AgentTelemetryCollector& instance() {
        static AgentTelemetryCollector collector;
        return collector;
    }

    void record(
        AgentTelemetryPhase phase,
        const std::string& task_id = "",
        const std::string& plan_id = "",
        const std::string& step_id = "",
        const std::string& details = ""
    ) {
        uint64_t now_ns = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count()
        );

        std::lock_guard<std::mutex> lock(mutex_);
        records_.push_back({phase, now_ns, task_id, plan_id, step_id, details});
    }

    [[nodiscard]] std::vector<AgentTelemetryRecord> records_for_task(const std::string& task_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<AgentTelemetryRecord> res;
        for (const auto& r : records_) {
            if (r.task_id == task_id) res.push_back(r);
        }
        return res;
    }

    [[nodiscard]] size_t total_records() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return records_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        records_.clear();
    }

private:
    AgentTelemetryCollector() = default;
    mutable std::mutex mutex_;
    std::vector<AgentTelemetryRecord> records_;
};

} // namespace vani::runtime::agent
