#pragma once

#include "../../contracts/agents/agent_execution_contracts.hpp"
#include "../../contracts/common/result.hpp"
#include "agent_memory.hpp"
#include <string>
#include <memory>
#include <vector>

namespace vani::runtime::agent {

class AgentModelProvider {
public:
    virtual ~AgentModelProvider() = default;

    [[nodiscard]] virtual std::string provider_id() const = 0;
    [[nodiscard]] virtual bool is_local() const = 0;
    [[nodiscard]] virtual bool is_available() const = 0;

    virtual contracts::Result<contracts::AgentPlan> generate_plan(
        const contracts::AgentRequest& request,
        const ShortTermTaskMemoryPtr& memory
    ) = 0;

    virtual contracts::Result<contracts::AgentPlan> replan(
        const contracts::AgentRequest& request,
        const contracts::AgentPlan& current_plan,
        const std::string& failed_step_id,
        const contracts::AgentObservation& failure_obs,
        const ShortTermTaskMemoryPtr& memory
    ) = 0;

    virtual contracts::Result<std::string> summarize_observation(
        const contracts::AgentObservation& observation
    ) = 0;
};

using AgentModelProviderPtr = std::shared_ptr<AgentModelProvider>;

// ============================================================================
// 1. Deterministic Rule-Based Planner (100% Offline & Local)
// ============================================================================
class DeterministicRulePlanner : public AgentModelProvider {
public:
    DeterministicRulePlanner() = default;
    ~DeterministicRulePlanner() override = default;

    [[nodiscard]] std::string provider_id() const override { return "deterministic_rule_planner"; }
    [[nodiscard]] bool is_local() const override { return true; }
    [[nodiscard]] bool is_available() const override { return true; }

    contracts::Result<contracts::AgentPlan> generate_plan(
        const contracts::AgentRequest& request,
        const ShortTermTaskMemoryPtr& memory
    ) override;

    contracts::Result<contracts::AgentPlan> replan(
        const contracts::AgentRequest& request,
        const contracts::AgentPlan& current_plan,
        const std::string& failed_step_id,
        const contracts::AgentObservation& failure_obs,
        const ShortTermTaskMemoryPtr& memory
    ) override;

    contracts::Result<std::string> summarize_observation(
        const contracts::AgentObservation& observation
    ) override;
};

// ============================================================================
// 2. Local LLM Planner (Out-of-Process Local Model / Worker Boundary)
// ============================================================================
class LocalLLMPlanner : public AgentModelProvider {
public:
    explicit LocalLLMPlanner(bool is_online = false, bool is_offline_mode = true);
    ~LocalLLMPlanner() override = default;

    [[nodiscard]] std::string provider_id() const override { return "local_llm_planner"; }
    [[nodiscard]] bool is_local() const override { return true; }
    [[nodiscard]] bool is_available() const override { return is_online_; }

    void set_online(bool online) { is_online_ = online; }

    contracts::Result<contracts::AgentPlan> generate_plan(
        const contracts::AgentRequest& request,
        const ShortTermTaskMemoryPtr& memory
    ) override;

    contracts::Result<contracts::AgentPlan> replan(
        const contracts::AgentRequest& request,
        const contracts::AgentPlan& current_plan,
        const std::string& failed_step_id,
        const contracts::AgentObservation& failure_obs,
        const ShortTermTaskMemoryPtr& memory
    ) override;

    contracts::Result<std::string> summarize_observation(
        const contracts::AgentObservation& observation
    ) override;

private:
    bool is_online_{false};
    bool is_offline_mode_{true};
};

// ============================================================================
// 3. Mock / Test Planner for Adversarial Testing
// ============================================================================
class MockOrTestPlanner : public AgentModelProvider {
public:
    MockOrTestPlanner() = default;
    ~MockOrTestPlanner() override = default;

    [[nodiscard]] std::string provider_id() const override { return "mock_or_test_planner"; }
    [[nodiscard]] bool is_local() const override { return true; }
    [[nodiscard]] bool is_available() const override { return true; }

    void set_plan_to_return(contracts::AgentPlan plan) { canned_plan_ = std::move(plan); }
    void set_simulate_offline_failure(bool fail) { sim_offline_fail_ = fail; }
    void set_simulate_resource_constrained(bool constrained) { sim_resource_constrained_ = constrained; }

    contracts::Result<contracts::AgentPlan> generate_plan(
        const contracts::AgentRequest& request,
        const ShortTermTaskMemoryPtr& memory
    ) override;

    contracts::Result<contracts::AgentPlan> replan(
        const contracts::AgentRequest& request,
        const contracts::AgentPlan& current_plan,
        const std::string& failed_step_id,
        const contracts::AgentObservation& failure_obs,
        const ShortTermTaskMemoryPtr& memory
    ) override;

    contracts::Result<std::string> summarize_observation(
        const contracts::AgentObservation& observation
    ) override;

private:
    contracts::AgentPlan canned_plan_;
    bool sim_offline_fail_{false};
    bool sim_resource_constrained_{false};
};

// ============================================================================
// Resource Governor (Native Win32 Memory Measurement & Constraints)
// ============================================================================
class ResourceGovernor {
public:
    static uint64_t get_real_available_ram_mb();
    static uint64_t get_real_total_ram_mb();
    static uint64_t get_real_process_memory_mb();

    static bool is_resource_constrained(uint64_t forced_ram_mb = 0) {
        if (forced_ram_mb > 0) {
            return forced_ram_mb < 500;
        }
        uint64_t avail = get_real_available_ram_mb();
        return (avail > 0 && avail < 500);
    }
};

} // namespace vani::runtime::agent
