#include "agent_audit_journal.hpp"
#include "agent_memory.hpp"
#include <chrono>
#include <sstream>

namespace vani::runtime::agent {

void AgentAuditJournal::log_step(
    const std::string& request_id,
    const std::string& plan_id,
    const contracts::AgentStep& step,
    const std::string& policy_decision,
    const contracts::AgentObservation& obs
) {
    std::lock_guard<std::mutex> lock(mutex_);
    AgentAuditEntry entry;
    entry.request_id = request_id;
    entry.plan_id = plan_id;
    entry.step_id = step.step_id;
    entry.capability = step.capability_id;
    entry.tool = step.tool_id;
    entry.policy_decision = policy_decision;
    entry.execution_result = obs.success ? "SUCCESS" : "FAILURE";
    entry.verification_result = obs.postcondition_verified ? "VERIFIED" : "VERIFICATION_FAILURE";
    entry.retry_count = (obs.attempts_taken > 1) ? (obs.attempts_taken - 1) : 0;
    entry.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    entry.final_outcome = obs.success ? obs.evidence : obs.error;

    // Build safe argument representation with secret scrubbing
    std::ostringstream ss;
    ss << "{";
    bool first = true;
    for (const auto& [k, v] : step.arguments) {
        if (!first) ss << ",";
        ss << "\"" << k << "\":\"" << ShortTermTaskMemory::scrub_sensitive_data(v) << "\"";
        first = false;
    }
    ss << "}";
    entry.safe_arguments = ss.str();

    entries_.push_back(std::move(entry));
}

void AgentAuditJournal::log_task_completion(
    const std::string& request_id,
    const std::string& plan_id,
    const contracts::PlanState& final_state,
    const std::string& outcome_summary
) {
    std::lock_guard<std::mutex> lock(mutex_);
    AgentAuditEntry entry;
    entry.request_id = request_id;
    entry.plan_id = plan_id;
    entry.step_id = "TASK_END";
    entry.capability = "agent.orchestration";
    entry.policy_decision = "N/A";
    entry.execution_result = (final_state == contracts::PlanState::Completed) ? "COMPLETED" : "INCOMPLETE";
    entry.verification_result = (final_state == contracts::PlanState::Completed) ? "VERIFIED" : "UNVERIFIED";
    entry.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    entry.final_outcome = std::string(contracts::to_string(final_state)) + ": " + outcome_summary;
    entries_.push_back(std::move(entry));
}

std::vector<AgentAuditEntry> AgentAuditJournal::entries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_;
}

std::vector<AgentAuditEntry> AgentAuditJournal::entries_for_request(const std::string& req_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AgentAuditEntry> res;
    for (const auto& e : entries_) {
        if (e.request_id == req_id) res.push_back(e);
    }
    return res;
}

size_t AgentAuditJournal::total_entries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

void AgentAuditJournal::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
}

} // namespace vani::runtime::agent
