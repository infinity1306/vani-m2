#pragma once

#include "repository.hpp"
#include "../runtime/session/session.hpp"
#include <unordered_map>
#include <mutex>
#include <memory>

namespace vani::storage {

class ISessionRepository : public IRepository<runtime::Session, contracts::SessionId> {
public:
    [[nodiscard]] virtual std::vector<runtime::Session> find_by_user(const std::string& user_id) const = 0;
};

class InMemorySessionRepository : public ISessionRepository {
public:
    contracts::Result<void> save(const runtime::Session& session) override {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_[session.session_id] = session;
        return contracts::Result<void>::ok();
    }

    [[nodiscard]] std::optional<runtime::Session> find_by_id(const contracts::SessionId& id) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(id);
        if (it != sessions_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::vector<runtime::Session> find_all() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<runtime::Session> result;
        result.reserve(sessions_.size());
        for (const auto& [id, sess] : sessions_) {
            result.push_back(sess);
        }
        return result;
    }

    [[nodiscard]] std::vector<runtime::Session> find_by_user(const std::string& user_id) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<runtime::Session> result;
        for (const auto& [id, sess] : sessions_) {
            if (sess.user_id == user_id) {
                result.push_back(sess);
            }
        }
        return result;
    }

    contracts::Result<void> remove(const contracts::SessionId& id) override {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.erase(id);
        return contracts::Result<void>::ok();
    }

    [[nodiscard]] size_t count() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return sessions_.size();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<contracts::SessionId, runtime::Session> sessions_;
};

using SessionRepositoryPtr = std::shared_ptr<ISessionRepository>;

} // namespace vani::storage
