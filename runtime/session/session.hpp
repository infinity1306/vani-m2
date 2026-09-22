#pragma once

#include "../../contracts/common/version.hpp"
#include <string>
#include <vector>
#include <unordered_set>
#include <chrono>

namespace vani::runtime {

struct Session {
    contracts::SessionId session_id;
    std::string user_id{"default_user"};
    contracts::DeviceId device_id{"local_desktop"};
    uint64_t started_at_ms{0};
    uint64_t last_activity_ms{0};
    std::vector<contracts::TaskId> active_task_ids;
    std::string context_reference;
    std::unordered_set<std::string> session_permissions;
    bool is_active{true};

    void touch() noexcept {
        last_activity_ms = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
    }
};

} // namespace vani::runtime
