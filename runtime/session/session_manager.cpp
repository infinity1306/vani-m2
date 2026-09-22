#include "session_manager.hpp"

namespace vani::runtime {

SessionManager::SessionManager(storage::SessionRepositoryPtr repository)
    : repository_(std::move(repository)) {}

SessionManager::~SessionManager() = default;

contracts::Result<contracts::SessionId> SessionManager::create_session(
    const std::string& user_id,
    const contracts::DeviceId& device_id,
    const std::string& context_ref
) {
    const auto now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    std::lock_guard<std::mutex> lock(mutex_);
    const auto session_id = "sess_" + std::to_string(now_ms) + "_" + std::to_string(session_counter_++);

    Session session{
        .session_id = session_id,
        .user_id = user_id,
        .device_id = device_id,
        .started_at_ms = now_ms,
        .last_activity_ms = now_ms,
        .active_task_ids = {},
        .context_reference = context_ref,
        .session_permissions = {},
        .is_active = true
    };

    sessions_[session_id] = session;
    if (repository_) {
        repository_->save(session);
    }

    return contracts::Result<contracts::SessionId>(session_id);
}

std::optional<Session> SessionManager::get_session(const contracts::SessionId& session_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        return it->second;
    }
    if (repository_) {
        return repository_->find_by_id(session_id);
    }
    return std::nullopt;
}

std::vector<Session> SessionManager::list_active_sessions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Session> result;
    for (const auto& [id, s] : sessions_) {
        if (s.is_active) {
            result.push_back(s);
        }
    }
    return result;
}

contracts::Result<void> SessionManager::touch_session(const contracts::SessionId& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::NotFound,
            "Session not found: " + session_id,
            "vani.runtime.session",
            false,
            contracts::ErrorCategory::Validation
        );
    }

    it->second.touch();
    if (repository_) {
        repository_->save(it->second);
    }
    return contracts::Result<void>::ok();
}

contracts::Result<void> SessionManager::add_task_to_session(
    const contracts::SessionId& session_id,
    const contracts::TaskId& task_id
) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::NotFound,
            "Session not found: " + session_id,
            "vani.runtime.session",
            false,
            contracts::ErrorCategory::Validation
        );
    }

    it->second.active_task_ids.push_back(task_id);
    it->second.touch();
    if (repository_) {
        repository_->save(it->second);
    }
    return contracts::Result<void>::ok();
}

contracts::Result<void> SessionManager::close_session(const contracts::SessionId& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::NotFound,
            "Session not found: " + session_id,
            "vani.runtime.session",
            false,
            contracts::ErrorCategory::Validation
        );
    }

    it->second.is_active = false;
    it->second.touch();
    if (repository_) {
        repository_->save(it->second);
    }
    return contracts::Result<void>::ok();
}

size_t SessionManager::active_session_count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [id, s] : sessions_) {
        if (s.is_active) count++;
    }
    return count;
}

} // namespace vani::runtime
