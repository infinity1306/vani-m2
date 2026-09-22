#include "file_watcher.hpp"
#include "path_security.hpp"
#include <chrono>

namespace vani::capabilities::system {

contracts::Result<uint32_t> FileWatcher::watch_path(
    const std::string& path,
    FileWatchCallback callback
) {
    if (PathSecurity::is_traversal_attack(path)) {
        return contracts::Result<uint32_t>::failure(
            contracts::ErrorCode::SecurityViolation, "Path traversal attack detected"
        );
    }

    std::string canonical = PathSecurity::canonicalize_path(path);
    if (PathSecurity::is_sensitive_location(canonical)) {
        return contracts::Result<uint32_t>::failure(
            contracts::ErrorCode::SecurityViolation, "Watching sensitive OS location is denied"
        );
    }

    std::lock_guard<std::mutex> lock(mutex_);
    uint32_t id = next_watch_id_++;
    watchers_[id] = WatchEntry{id, canonical, std::move(callback)};
    return contracts::Result<uint32_t>::success(id);
}

contracts::Result<void> FileWatcher::unwatch_path(uint32_t watch_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = watchers_.find(watch_id);
    if (it != watchers_.end()) {
        watchers_.erase(it);
        return contracts::Result<void>::success();
    }
    return contracts::Result<void>::failure(
        contracts::ErrorCode::NotFound, "Watch ID not found"
    );
}

void FileWatcher::notify_event(const std::string& path, contracts::FileWatchEvent event) {
    std::string canonical = PathSecurity::canonicalize_path(path);
    uint64_t now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    std::vector<FileWatchCallback> matched_callbacks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [id, entry] : watchers_) {
            if (canonical.rfind(entry.path, 0) == 0) {
                matched_callbacks.push_back(entry.callback);
            }
        }
    }

    contracts::FileWatchNotification notification{canonical, event, now_ms};
    for (const auto& cb : matched_callbacks) {
        if (cb) {
            cb(notification);
        }
    }
}

size_t FileWatcher::active_watchers_count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return watchers_.size();
}

} // namespace vani::capabilities::system
