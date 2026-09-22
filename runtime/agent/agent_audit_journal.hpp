#pragma once

#include "../../contracts/agents/agent_execution_contracts.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <memory>

namespace vani::runtime::agent {

struct AgentAuditEntry {
    std::string request_id;
    std::string plan_id;
    std::string step_id;
    std::string capability;
    std::string tool;
    std::string safe_arguments;
    std::string policy_decision;
    std::string execution_result;
    std::string verification_result;
    uint32_t retry_count{0};
    uint64_t timestamp_ms{0};
    std::string final_outcome;
};

class AgentAuditJournal {
public:
    AgentAuditJournal() = default;
    ~AgentAuditJournal() = default;

    void log_step(
        const std::string& request_id,
        const std::string& plan_id,
        const contracts::AgentStep& step,
        const std::string& policy_decision,
        const contracts::AgentObservation& obs
    );

    void log_task_completion(
        const std::string& request_id,
        const std::string& plan_id,
        const contracts::PlanState& final_state,
        const std::string& outcome_summary
    );

    [[nodiscard]] std::vector<AgentAuditEntry> entries() const;
    [[nodiscard]] std::vector<AgentAuditEntry> entries_for_request(const std::string& req_id) const;
    [[nodiscard]] size_t total_entries() const;
    void clear();

private:
    mutable std::mutex mutex_;
    std::vector<AgentAuditEntry> entries_;
};

using AgentAuditJournalPtr = std::shared_ptr<AgentAuditJournal>;

} // namespace vani::runtime::agent
