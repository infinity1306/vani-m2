#include "permission_service.hpp"

namespace vani::runtime {

PermissionService::PermissionService() = default;
PermissionService::~PermissionService() = default;

void PermissionService::grant_permission(
    const std::string& actor_id,
    const std::string& capability_permission,
    PermissionScope scope,
    const std::string& session_id,
    const std::string& task_id,
    uint64_t duration_ms,
    const std::string& reason
) {
    const auto now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    std::lock_guard<std::mutex> lock(mutex_);
    grants_[actor_id].push_back(PermissionGrant{
        .id = "perm_" + std::to_string(now_ms) + "_" + std::to_string(grants_[actor_id].size()),
        .actor_id = actor_id,
        .permission = capability_permission,
        .scope = scope,
        .session_id = session_id,
        .task_id = task_id,
        .granted_at_ms = now_ms,
        .expires_at_ms = duration_ms > 0 ? now_ms + duration_ms : 0,
        .reason = reason
    });
}

void PermissionService::revoke_permission(
    const std::string& actor_id,
    const std::string& capability_permission
) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = grants_.find(actor_id);
    if (it != grants_.end()) {
        auto& list = it->second;
        for (auto git = list.begin(); git != list.end();) {
            if (git->permission == capability_permission) {
                git = list.erase(git);
            } else {
                ++git;
            }
        }
    }
}

bool PermissionService::has_permission(
    const std::string& actor_id,
    const std::string& capability_permission,
    const std::string& session_id,
    const std::string& task_id
) const {
    const auto now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    std::lock_guard<std::mutex> lock(mutex_);

    // Check actor-specific grants
    auto check_list = [&](const std::string& actor) {
        auto it = grants_.find(actor);
        if (it == grants_.end()) return false;

        for (const auto& grant : it->second) {
            // Expiry check
            if (grant.expires_at_ms > 0 && now_ms >= grant.expires_at_ms) {
                continue;
            }

            // Scope check
            if (grant.scope == PermissionScope::Session && !session_id.empty() && grant.session_id != session_id) {
                continue;
            }
            if (grant.scope == PermissionScope::Task && !task_id.empty() && grant.task_id != task_id) {
                continue;
            }

            // Match exact or wildcard
            if (grant.permission == capability_permission || grant.permission == "*") {
                return true;
            }
            if (grant.permission.ends_with(".*")) {
                const auto prefix = grant.permission.substr(0, grant.permission.size() - 2);
                if (capability_permission.starts_with(prefix)) {
                    return true;
                }
            }
        }
        return false;
    };

    return check_list(actor_id) || check_list("*");
}

std::vector<PermissionGrant> PermissionService::list_grants(const std::string& actor_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = grants_.find(actor_id);
    if (it != grants_.end()) {
        return it->second;
    }
    return {};
}

void PermissionService::cleanup_expired() {
    const auto now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [actor, list] : grants_) {
        for (auto it = list.begin(); it != list.end();) {
            if (it->expires_at_ms > 0 && now_ms >= it->expires_at_ms) {
                it = list.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void PermissionService::clear_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    grants_.clear();
}

} // namespace vani::runtime
