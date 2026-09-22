#include "execution_scheduler.hpp"
#include "agent_telemetry.hpp"
#include <chrono>
#include <thread>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <queue>

namespace vani::runtime::agent {

ExecutionScheduler::ExecutionScheduler(
    capabilities::system::ToolGatewayPtr gateway,
    PolicyBoundaryPtr policy_boundary,
    PostconditionVerifierPtr verifier
) : gateway_(std::move(gateway)),
    policy_boundary_(std::move(policy_boundary)),
    verifier_(std::move(verifier)) {}

contracts::AgentExecutionResult ExecutionScheduler::execute_plan(
    const contracts::AgentPlan& plan,
    const contracts::AgentRequest& request,
    StepStatusCallback callback
) {
    auto t_start = std::chrono::steady_clock::now();
    contracts::AgentExecutionResult final_res;
    final_res.request_id = request.request_id;
    final_res.plan_id = plan.plan_id;
    final_res.final_state = contracts::PlanState::Executing;

    // Build map of steps
    std::unordered_map<std::string, contracts::AgentStep> step_map;
    std::unordered_map<std::string, std::vector<std::string>> adj;
    std::unordered_map<std::string, int> in_degree;

    for (const auto& s : plan.steps) {
        step_map[s.step_id] = s;
        in_degree[s.step_id] = 0;
    }

    for (const auto& s : plan.steps) {
        for (const auto& dep : s.dependencies) {
            adj[dep].push_back(s.step_id);
            in_degree[s.step_id]++;
        }
    }

    for (const auto& [from, to] : plan.dependencies) {
        adj[from].push_back(to);
        in_degree[to]++;
    }

    // Topological execution queue
    std::queue<std::string> ready_queue;
    for (const auto& [id, deg] : in_degree) {
        if (deg == 0) ready_queue.push(id);
    }

    std::unordered_map<std::string, contracts::AgentObservation> step_results;
    std::unordered_set<std::string> failed_steps;

    while (!ready_queue.empty()) {
        std::string curr_step_id = ready_queue.front();
        ready_queue.pop();

        const auto& step = step_map[curr_step_id];

        // 1. Cancellation check
        if (request.cancellation_token.is_cancelled()) {
            final_res.final_state = contracts::PlanState::Cancelled;
            final_res.error_message = "Execution cancelled by user token";
            AgentTelemetryCollector::instance().record(
                AgentTelemetryPhase::A13_Cancellation, request.request_id, plan.plan_id, step.step_id, "User cancelled"
            );
            break;
        }

        // 2. Global Timeout check
        auto now = std::chrono::steady_clock::now();
        uint64_t elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - t_start).count();
        if (request.resource_budget.global_timeout_ms > 0 && elapsed_ms > request.resource_budget.global_timeout_ms) {
            final_res.final_state = contracts::PlanState::Timeout;
            final_res.error_message = "Global execution timeout exceeded (" + std::to_string(elapsed_ms) + " ms)";
            break;
        }

        // 3. Check dependencies
        bool prereq_failed = false;
        for (const auto& dep : step.dependencies) {
            if (failed_steps.count(dep) || (step_results.count(dep) && !step_results[dep].success)) {
                prereq_failed = true;
                break;
            }
        }

        if (prereq_failed) {
            failed_steps.insert(curr_step_id);
            contracts::AgentObservation obs;
            obs.step_id = curr_step_id;
            obs.success = false;
            obs.error = "BLOCKED_DEPENDENCY: Prerequisite step failed";
            obs.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            step_results[curr_step_id] = obs;
            final_res.observations.push_back(obs);
            final_res.final_state = contracts::PlanState::BlockedDependency;
            final_res.error_message = obs.error;
            break;
        }

        AgentTelemetryCollector::instance().record(
            AgentTelemetryPhase::A5_StepStart, request.request_id, plan.plan_id, step.step_id
        );

        // 4. Policy check
        AgentTelemetryCollector::instance().record(
            AgentTelemetryPhase::A4_PolicyCheck, request.request_id, plan.plan_id, step.step_id
        );

        if (policy_boundary_) {
            auto pol_eval = policy_boundary_->evaluate_step(step, request, plan.plan_id);
            if (pol_eval.requires_confirmation) {
                bool confirmed = policy_boundary_->prompt_confirmation(pol_eval.confirmation_payload);
                if (!confirmed) {
                    failed_steps.insert(curr_step_id);
                    contracts::AgentObservation obs;
                    obs.step_id = curr_step_id;
                    obs.success = false;
                    obs.error = "POLICY_DENIED: User rejected confirmation request";
                    obs.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    step_results[curr_step_id] = obs;
                    final_res.observations.push_back(obs);
                    final_res.final_state = contracts::PlanState::BlockedPolicy;
                    final_res.error_message = obs.error;
                    break;
                }
            } else if (!pol_eval.allowed) {
                failed_steps.insert(curr_step_id);
                contracts::AgentObservation obs;
                obs.step_id = curr_step_id;
                obs.success = false;
                obs.error = "POLICY_DENIED: " + pol_eval.reason;
                obs.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                step_results[curr_step_id] = obs;
                final_res.observations.push_back(obs);
                final_res.final_state = contracts::PlanState::BlockedPolicy;
                final_res.error_message = obs.error;
                break;
            }
        }

        // 5. Tool execution with bounded retries
        uint32_t attempts = 0;
        uint32_t max_attempts = std::max<uint32_t>(1, step.retry_policy_max_attempts);
        contracts::ToolResult tool_res;
        bool executed_ok = false;

        while (attempts < max_attempts) {
            attempts++;
            final_res.total_tool_calls++;

            AgentTelemetryCollector::instance().record(
                AgentTelemetryPhase::A6_ToolRequest, request.request_id, plan.plan_id, step.step_id,
                "Attempt " + std::to_string(attempts)
            );

            // Check simulated transient failure
            if (transient_failures_remaining_ > 0) {
                transient_failures_remaining_--;
                tool_res.success = false;
                tool_res.execution_log = "Transient device communication timeout";
                final_res.total_retries++;
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }

            if (gateway_) {
                capabilities::system::ToolExecutionPipelineContext pctx;
                pctx.capability_id = step.capability_id;
                pctx.tool_id = step.tool_id;
                pctx.task_id = request.request_id;
                pctx.actor_id = "agent.planner";
                pctx.cancellation_token = request.cancellation_token;

                std::ostringstream ss;
                ss << "{";
                bool first = true;
                for (const auto& [k, v] : step.arguments) {
                    if (!first) ss << ",";
                    ss << "\"" << k << "\":\"" << v << "\"";
                    first = false;
                }
                ss << "}";
                pctx.arguments_json = ss.str();

                auto exec_r = gateway_->execute(pctx);
                if (exec_r.is_ok()) {
                    tool_res = exec_r.value();
                } else {
                    tool_res.success = false;
                    tool_res.execution_log = exec_r.error().message;
                }
            } else {
                tool_res.success = true;
                tool_res.output_json = "{\"status\":\"ok\"}";
            }

            AgentTelemetryCollector::instance().record(
                AgentTelemetryPhase::A7_ToolComplete, request.request_id, plan.plan_id, step.step_id
            );

            if (tool_res.success) {
                executed_ok = true;
                break;
            } else {
                if (attempts < max_attempts) {
                    final_res.total_retries++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
            }
        }

        // 6. Postcondition verification
        AgentTelemetryCollector::instance().record(
            AgentTelemetryPhase::A8_Verification, request.request_id, plan.plan_id, step.step_id
        );

        VerificationResult vres;
        if (verifier_) {
            vres = verifier_->verify(step, tool_res);
        } else {
            vres.verified = tool_res.success;
            vres.evidence = "No verifier attached, defaulted to tool status";
        }

        contracts::AgentObservation obs;
        obs.step_id = curr_step_id;
        obs.success = executed_ok && vres.verified;
        obs.result_data = tool_res.output_json;
        obs.postcondition_verified = vres.verified;
        obs.evidence = vres.evidence;
        obs.attempts_taken = attempts;
        obs.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        if (!obs.success) {
            obs.error = !executed_ok ? tool_res.execution_log : ("VERIFICATION_FAILURE: " + vres.failure_reason);
            failed_steps.insert(curr_step_id);
            step_results[curr_step_id] = obs;
            final_res.observations.push_back(obs);
            final_res.total_steps_executed++;
            final_res.final_state = contracts::PlanState::Failed;
            final_res.error_message = obs.error;
            if (callback) callback(step, obs);
            break;
        }

        AgentTelemetryCollector::instance().record(
            AgentTelemetryPhase::A9_Observation, request.request_id, plan.plan_id, step.step_id, obs.evidence
        );

        step_results[curr_step_id] = obs;
        final_res.observations.push_back(obs);
        final_res.total_steps_executed++;

        if (callback) callback(step, obs);

        // Advance ready queue for dependents
        for (const auto& nxt : adj[curr_step_id]) {
            if (--in_degree[nxt] == 0) {
                ready_queue.push(nxt);
            }
        }
    }

    auto t_end = std::chrono::steady_clock::now();
    final_res.duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();

    if (final_res.final_state == contracts::PlanState::Executing && final_res.total_steps_executed == plan.steps.size()) {
        final_res.final_state = contracts::PlanState::Completed;
        final_res.verified = true;
        final_res.final_response = "All " + std::to_string(final_res.total_steps_executed) + " plan steps completed and verified successfully.";
        AgentTelemetryCollector::instance().record(
            AgentTelemetryPhase::A11_TaskComplete, request.request_id, plan.plan_id
        );
    } else if (final_res.final_state != contracts::PlanState::Cancelled &&
               final_res.final_state != contracts::PlanState::Timeout &&
               final_res.final_state != contracts::PlanState::BlockedPolicy &&
               final_res.final_state != contracts::PlanState::BlockedDependency) {
        final_res.final_state = contracts::PlanState::Failed;
        AgentTelemetryCollector::instance().record(
            AgentTelemetryPhase::A12_TaskFailed, request.request_id, plan.plan_id, "", final_res.error_message
        );
    }

    return final_res;
}

} // namespace vani::runtime::agent
