#pragma once

#include "../../contracts/tools/risk_level.hpp"
#include <string>
#include <string_view>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace vani::runtime {

enum class PolicyDecision : uint8_t {
    Allow,
    AllowWithAudit,
    RequireConfirmation,
    Deny
};

[[nodiscard]] constexpr std::string_view to_string(PolicyDecision decision) noexcept {
    switch (decision) {
        case PolicyDecision::Allow: return "ALLOW";
        case PolicyDecision::AllowWithAudit: return "ALLOW_WITH_AUDIT";
        case PolicyDecision::RequireConfirmation: return "REQUIRE_CONFIRMATION";
        case PolicyDecision::Deny: return "DENY";
        default: return "DENY";
    }
}

enum class PolicyPriority : uint8_t {
    CapabilityDefault = 0,
    AgentPolicy = 1,
    SessionPolicy = 2,
    UserPolicy = 3,
    SystemSafety = 4
};

struct PolicyContext {
    std::string actor_id;              // e.g. "agent.odysseus", "user", "plugin.unknown"
    std::string capability_id;         // e.g. "terminal.execute", "filesystem.write"
    contracts::RiskLevel risk_level{contracts::RiskLevel::Low};
    std::string user_id{"default_user"};
    std::string session_id;
    std::string task_id;
    std::string resource_path;
    bool is_local_execution{true};
    bool is_user_present{true};
    bool data_is_sensitive{false};
    std::unordered_map<std::string, std::string> metadata{};
};

class PolicyEngine {
public:
    PolicyEngine();
    ~PolicyEngine();

    [[nodiscard]] PolicyDecision evaluate(const PolicyContext& context) const;

    void set_strict_mode(bool strict) noexcept;
    [[nodiscard]] bool is_strict_mode() const noexcept;

    void add_override(const std::string& capability_id, PolicyDecision decision);
    void remove_override(const std::string& capability_id);

private:
    bool strict_mode_{true};
    mutable std::mutex mutex_;
    std::unordered_map<std::string, PolicyDecision> capability_overrides_;
};

using PolicyEnginePtr = std::shared_ptr<PolicyEngine>;

} // namespace vani::runtime
