#include "router.hpp"

namespace vani::runtime {

Router::Router(
    CapabilityRegistryPtr registry,
    PolicyEnginePtr policy_engine,
    PermissionServicePtr permission_service
) : registry_(std::move(registry)),
    policy_engine_(std::move(policy_engine)),
    permission_service_(std::move(permission_service)) {}

Router::~Router() = default;

contracts::Result<RoutePlan> Router::resolve_route(const contracts::TaskSpecification& task_spec) {
    if (!registry_) {
        return contracts::Result<RoutePlan>::err(
            contracts::ErrorCode::DependencyError,
            "Capability registry not initialized in Router",
            "vani.runtime.router"
        );
    }

    RoutePlan plan{
        .task_id = task_spec.task_id,
        .policy_decision = PolicyDecision::Allow,
        .is_executable = true,
        .rationale = ""
    };

    // 1. Resolve requested capabilities against the registry
    std::string candidate_provider;
    contracts::RiskLevel highest_risk = contracts::RiskLevel::Low;

    for (const auto& cap_id : task_spec.requested_capabilities) {
        auto cap = registry_->get_capability(cap_id);
        if (!cap.has_value()) {
            return contracts::Result<RoutePlan>::err(
                contracts::ErrorCode::NotFound,
                "Requested capability not registered: " + cap_id,
                "vani.runtime.router"
            );
        }
        if (cap->availability != CapabilityAvailability::Available) {
            return contracts::Result<RoutePlan>::err(
                contracts::ErrorCode::Unavailable,
                "Capability currently unavailable: " + cap_id,
                "vani.runtime.router"
            );
        }

        plan.matched_capabilities.push_back(cap_id);
        if (candidate_provider.empty()) {
            candidate_provider = cap->provider_id;
        }
        if (static_cast<uint8_t>(cap->risk_level) > static_cast<uint8_t>(highest_risk)) {
            highest_risk = cap->risk_level;
        }
    }

    // 2. Evaluate Policy
    if (policy_engine_) {
        PolicyContext pctx{
            .actor_id = candidate_provider.empty() ? "system.core" : candidate_provider,
            .capability_id = plan.matched_capabilities.empty() ? "general" : plan.matched_capabilities.front(),
            .risk_level = highest_risk,
            .is_local_execution = true,
            .is_user_present = true
        };
        plan.policy_decision = policy_engine_->evaluate(pctx);
        if (plan.policy_decision == PolicyDecision::Deny) {
            plan.is_executable = false;
            plan.rationale = "Execution denied by central security policy.";
            return contracts::Result<RoutePlan>(plan);
        }
    }

    // 3. Fallback to assigned agent if specified in task_spec
    if (candidate_provider.empty() && !task_spec.assigned_agent_id.empty()) {
        candidate_provider = task_spec.assigned_agent_id;
    }

    plan.selected_provider_or_agent_id = candidate_provider.empty() ? "system.default_worker" : candidate_provider;
    plan.rationale = "Resolved route to " + plan.selected_provider_or_agent_id + " based on requested capabilities.";

    return contracts::Result<RoutePlan>(plan);
}

} // namespace vani::runtime
