#include "agent_controller.hpp"
#include "agent_telemetry.hpp"
#include <iostream>

namespace vani::runtime::agent {

AgentController::AgentController(
    capabilities::system::ToolGatewayPtr gateway,
    runtime::PolicyEnginePtr policy_engine,
    AgentModelProviderPtr model_provider
) : gateway_(std::move(gateway)),
    policy_engine_(std::move(policy_engine)),
    model_provider_(std::move(model_provider)) {

    if (!model_provider_) {
        model_provider_ = std::make_shared<DeterministicRulePlanner>();
    }

    policy_boundary_ = std::make_shared<PolicyBoundary>(policy_engine_);
    verifier_ = std::make_shared<PostconditionVerifier>(gateway_);
    scheduler_ = std::make_shared<ExecutionScheduler>(gateway_, policy_boundary_, verifier_);
    memory_ = std::make_shared<ShortTermTaskMemory>();
    audit_journal_ = std::make_shared<AgentAuditJournal>();
}

ClassificationDecision AgentController::classify_request(
    const std::string& raw_text,
    const std::string& normalized_text,
    const std::string& intent
) const {
    return classifier_.classify(raw_text, normalized_text, intent);
}

contracts::AgentExecutionResult AgentController::execute_goal(const contracts::AgentRequest& request) {
    contracts::AgentExecutionResult result;
    result.request_id = request.request_id;
    result.final_state = contracts::PlanState::Created;

    // Crash isolation wrapper around entire agentic execution
    try {
        std::string safe_goal = ShortTermTaskMemory::scrub_sensitive_data(request.goal);
        AgentTelemetryCollector::instance().record(
            AgentTelemetryPhase::A0_AgentRequest, request.request_id, "", "", safe_goal
        );

        if (require_real_model_ && (!model_provider_ || model_provider_->provider_id() == "deterministic_rule_planner")) {
            result.final_state = contracts::PlanState::Failed;
            result.error_message = "REAL_MODEL_REQUIRED: Real AgentModelProvider is required but missing or deterministic.";
            result.final_response = "Execution blocked: real local LLM model provider is required.";
            AgentTelemetryCollector::instance().record(
                AgentTelemetryPhase::A12_TaskFailed, request.request_id, "", "", result.error_message
            );
            audit_journal_->log_task_completion(request.request_id, "", result.final_state, result.error_message);
            return result;
        }

        // 1. Resource check
        if (ResourceGovernor::is_resource_constrained(simulated_free_ram_mb_)) {
            result.final_state = contracts::PlanState::BlockedResource;
            result.error_message = "RESOURCE_CONSTRAINED: Available host memory is below minimum execution threshold.";
            result.final_response = "Execution blocked due to insufficient system resources.";
            AgentTelemetryCollector::instance().record(
                AgentTelemetryPhase::A12_TaskFailed, request.request_id, "", "", result.error_message
            );
            audit_journal_->log_task_completion(request.request_id, "", result.final_state, result.error_message);
            return result;
        }

        // 2. Planning phase
        AgentTelemetryCollector::instance().record(
            AgentTelemetryPhase::A1_PlanningStart, request.request_id, "", "", "Provider: " + model_provider_->provider_id()
        );

        auto plan_res = model_provider_->generate_plan(request, memory_);
        if (!plan_res.is_ok()) {
            result.final_state = contracts::PlanState::Failed;
            result.error_message = plan_res.error().message;
            result.final_response = "Planning failed: " + plan_res.error().message;
            AgentTelemetryCollector::instance().record(
                AgentTelemetryPhase::A12_TaskFailed, request.request_id, "", "", result.error_message
            );
            audit_journal_->log_task_completion(request.request_id, "", result.final_state, result.error_message);
            return result;
        }

        contracts::AgentPlan plan = plan_res.value();
        last_initial_plan_ = plan;
        result.plan_id = plan.plan_id;

        AgentTelemetryCollector::instance().record(
            AgentTelemetryPhase::A2_PlanningComplete, request.request_id, plan.plan_id, "",
            "Steps: " + std::to_string(plan.steps.size())
        );

        // 3. Plan validation phase
        auto val_res = validator_.validate_plan(plan, request.resource_budget);
        if (!val_res.is_valid) {
            result.final_state = contracts::PlanState::Failed;
            result.error_message = "PLAN_REJECTED: " + val_res.error_message;
            result.final_response = "Plan validation failed: " + val_res.error_message;
            AgentTelemetryCollector::instance().record(
                AgentTelemetryPhase::A12_TaskFailed, request.request_id, plan.plan_id, "", result.error_message
            );
            audit_journal_->log_task_completion(request.request_id, plan.plan_id, result.final_state, result.error_message);
            return result;
        }

        AgentTelemetryCollector::instance().record(
            AgentTelemetryPhase::A3_ValidationComplete, request.request_id, plan.plan_id
        );

        // 4. Execution scheduling
        auto step_callback = [&](const contracts::AgentStep& step, const contracts::AgentObservation& obs) {
            memory_->add_observation(obs);
            audit_journal_->log_step(request.request_id, plan.plan_id, step, "ALLOW", obs);
        };

        result = scheduler_->execute_plan(plan, request, step_callback);

        // 5. Replanning if failure is recoverable and replans remain
        uint32_t replans_done = 0;
        while (result.final_state == contracts::PlanState::Failed &&
               replans_done < request.resource_budget.max_replans) {

            // Find the failed step observation
            std::string failed_step_id;
            contracts::AgentObservation failed_obs;
            for (const auto& o : result.observations) {
                if (!o.success) {
                    failed_step_id = o.step_id;
                    failed_obs = o;
                    break;
                }
            }

            if (failed_step_id.empty()) break;

            AgentTelemetryCollector::instance().record(
                AgentTelemetryPhase::A10_Replan, request.request_id, plan.plan_id, failed_step_id,
                "Replan attempt " + std::to_string(replans_done + 1)
            );

            if (!model_provider_) {
                result.error_message = "PLAN_REPLAN_FAILED: Model provider is null during replan";
                break;
            }

            auto replan_res = model_provider_->replan(request, plan, failed_step_id, failed_obs, memory_);
            if (!replan_res.is_ok()) {
                result.error_message = "REPLAN_FAILED: " + replan_res.error().message;
                AgentTelemetryCollector::instance().record(
                    AgentTelemetryPhase::A12_TaskFailed, request.request_id, plan.plan_id, failed_step_id, result.error_message
                );
                break;
            }

            replans_done++;
            result.total_replans = replans_done;
            plan = replan_res.value();
            last_replanned_plan_ = plan;

            AgentTelemetryCollector::instance().record(
                AgentTelemetryPhase::A2_PlanningComplete, request.request_id, plan.plan_id, "",
                "Replanned steps: " + std::to_string(plan.steps.size())
            );

            // Re-validate replanned plan
            auto reval = validator_.validate_plan(plan, request.resource_budget);
            if (!reval.is_valid) {
                result.error_message = "PLAN_REJECTED on replan: " + reval.error_message;
                AgentTelemetryCollector::instance().record(
                    AgentTelemetryPhase::A12_TaskFailed, request.request_id, plan.plan_id, "", result.error_message
                );
                break;
            }

            // Execute replanned plan
            result = scheduler_->execute_plan(plan, request, step_callback);
            result.total_replans = replans_done;
        }

        // 6. User-facing response formulation
        if (result.final_state == contracts::PlanState::Completed) {
            result.verified = true;
            result.final_response = "All tasks completed successfully and verified on system.";
        } else if (result.final_state == contracts::PlanState::BlockedPolicy) {
            result.final_response = "Action stopped because policy security rules disallow it.";
        } else if (result.final_state == contracts::PlanState::BlockedDependency) {
            result.final_response = "Next step was blocked because a required prerequisite step failed.";
        } else if (result.final_state == contracts::PlanState::Cancelled) {
            result.final_response = "Agent task was cancelled by user.";
        } else if (result.final_state == contracts::PlanState::Timeout) {
            result.final_response = "Agent task exceeded execution time limit and was stopped.";
        } else {
            result.final_response = "Agent execution stopped: " + result.error_message;
        }

        audit_journal_->log_task_completion(
            request.request_id, plan.plan_id, result.final_state, result.final_response
        );

    } catch (const std::exception& ex) {
        // Guarantee VANI core crash isolation
        result.final_state = contracts::PlanState::Failed;
        result.error_message = std::string("Unhandled exception in agent controller: ") + ex.what();
        result.final_response = "Agent execution encountered an internal error and was safely contained.";
        audit_journal_->log_task_completion(request.request_id, "", result.final_state, result.error_message);
    }

    return result;
}

} // namespace vani::runtime::agent
