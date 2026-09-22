#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>
#include <chrono>

namespace vani::runtime {

enum class PermissionScope : uint8_t {
    Request,
    Task,
    Session,
    Persistent
};

struct PermissionGrant {
    std::string id;
    std::string actor_id;
    std::string permission;
    PermissionScope scope{PermissionScope::Persistent};
    std::string session_id;
    std::string task_id;
    uint64_t granted_at_ms{0};
    uint64_t expires_at_ms{0};
    std::string reason;
};

class PermissionService {
public:
    PermissionService();
    ~PermissionService();

    void grant_permission(
        const std::string& actor_id,
        const std::string& capability_permission,
        PermissionScope scope = PermissionScope::Persistent,
        const std::string& session_id = "",
        const std::string& task_id = "",
        uint64_t duration_ms = 0,
        const std::string& reason = "Explicit authorization"
    );

    void revoke_permission(
        const std::string& actor_id,
        const std::string& capability_permission
    );

    [[nodiscard]] bool has_permission(
        const std::string& actor_id,
        const std::string& capability_permission,
        const std::string& session_id = "",
        const std::string& task_id = ""
    ) const;

    [[nodiscard]] std::vector<PermissionGrant> list_grants(const std::string& actor_id) const;

    void cleanup_expired();
    void clear_all();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<PermissionGrant>> grants_;
};

using PermissionServicePtr = std::shared_ptr<PermissionService>;

} // namespace vani::runtime
