#pragma once

#include "../capability_registry/capability_registry.hpp"
#include "../policy/policy_engine.hpp"
#include "../permissions/permission_service.hpp"
#include "../../contracts/tasks/task.hpp"
#include "../../contracts/common/result.hpp"
#include <string>
#include <vector>
#include <memory>

namespace vani::runtime {

struct RoutePlan {
    contracts::TaskId task_id;
    std::string selected_provider_or_agent_id;
    std::vector<contracts::CapabilityId> matched_capabilities;
    PolicyDecision policy_decision{PolicyDecision::Allow};
    bool is_executable{true};
    std::string rationale;
};

class Router {
public:
    Router(
        CapabilityRegistryPtr registry,
        PolicyEnginePtr policy_engine,
        PermissionServicePtr permission_service
    );
    ~Router();

    contracts::Result<RoutePlan> resolve_route(const contracts::TaskSpecification& task_spec);

private:
    CapabilityRegistryPtr registry_;
    PolicyEnginePtr policy_engine_;
    PermissionServicePtr permission_service_;
};

using RouterPtr = std::shared_ptr<Router>;

} // namespace vani::runtime
