#include "policy_engine.hpp"

namespace vani::runtime {

PolicyEngine::PolicyEngine() = default;
PolicyEngine::~PolicyEngine() = default;

PolicyDecision PolicyEngine::evaluate(const PolicyContext& context) const {
    std::lock_guard<std::mutex> lock(mutex_);

    // 1. System Safety Level Rules (Top Priority)
    // Critical risk always requires confirmation or deny
    if (context.risk_level == contracts::RiskLevel::Critical) {
        if (!context.is_user_present) {
            return PolicyDecision::Deny;
        }
        return PolicyDecision::RequireConfirmation;
    }

    // Untrusted or unverified actor attempting sensitive access
    if (context.actor_id.starts_with("plugin.unverified") ||
        context.actor_id.starts_with("plugin.unknown")) {
        if (context.risk_level >= contracts::RiskLevel::Medium || context.data_is_sensitive) {
            return PolicyDecision::Deny;
        }
        return PolicyDecision::RequireConfirmation;
    }

    // 2. Explicit Overrides
    auto override_it = capability_overrides_.find(context.capability_id);
    if (override_it != capability_overrides_.end()) {
        return override_it->second;
    }

    // 3. User & Session Level Policy Evaluations
    if (context.risk_level == contracts::RiskLevel::Safe) {
        return PolicyDecision::Allow;
    }

    if (context.risk_level == contracts::RiskLevel::Low) {
        if (context.data_is_sensitive) {
            return PolicyDecision::AllowWithAudit;
        }
        return PolicyDecision::Allow;
    }

    if (context.risk_level == contracts::RiskLevel::Medium) {
        return PolicyDecision::AllowWithAudit;
    }

    if (context.risk_level == contracts::RiskLevel::High) {
        if (strict_mode_) {
            return PolicyDecision::RequireConfirmation;
        }
        return PolicyDecision::AllowWithAudit;
    }

    return PolicyDecision::Deny;
}

void PolicyEngine::set_strict_mode(bool strict) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    strict_mode_ = strict;
}

bool PolicyEngine::is_strict_mode() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return strict_mode_;
}

void PolicyEngine::add_override(const std::string& capability_id, PolicyDecision decision) {
    std::lock_guard<std::mutex> lock(mutex_);
    capability_overrides_[capability_id] = decision;
}

void PolicyEngine::remove_override(const std::string& capability_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    capability_overrides_.erase(capability_id);
}

} // namespace vani::runtime
