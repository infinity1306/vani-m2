#pragma once

#include "session.hpp"
#include "../../contracts/common/result.hpp"
#include "../../storage/session_repository.hpp"
#include <unordered_map>
#include <mutex>
#include <memory>
#include <optional>
#include <vector>

namespace vani::runtime {

class SessionManager {
public:
    explicit SessionManager(storage::SessionRepositoryPtr repository = nullptr);
    ~SessionManager();

    contracts::Result<contracts::SessionId> create_session(
        const std::string& user_id = "default_user",
        const contracts::DeviceId& device_id = "local_desktop",
        const std::string& context_ref = ""
    );

    [[nodiscard]] std::optional<Session> get_session(const contracts::SessionId& session_id) const;
    [[nodiscard]] std::vector<Session> list_active_sessions() const;

    contracts::Result<void> touch_session(const contracts::SessionId& session_id);
    contracts::Result<void> add_task_to_session(const contracts::SessionId& session_id, const contracts::TaskId& task_id);
    contracts::Result<void> close_session(const contracts::SessionId& session_id);

    [[nodiscard]] size_t active_session_count() const noexcept;

private:
    mutable std::mutex mutex_;
    storage::SessionRepositoryPtr repository_;
    std::unordered_map<contracts::SessionId, Session> sessions_;
    uint64_t session_counter_{1};
};

using SessionManagerPtr = std::shared_ptr<SessionManager>;

} // namespace vani::runtime
