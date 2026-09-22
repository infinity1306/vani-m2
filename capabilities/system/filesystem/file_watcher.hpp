#pragma once

#include "../../../contracts/system/system_contracts.hpp"
#include "../../../contracts/common/result.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <functional>

namespace vani::capabilities::system {

using FileWatchCallback = std::function<void(const contracts::FileWatchNotification&)>;

class FileWatcher {
public:
    FileWatcher() = default;
    ~FileWatcher() = default;

    contracts::Result<uint32_t> watch_path(
        const std::string& path,
        FileWatchCallback callback
    );
    contracts::Result<void> unwatch_path(uint32_t watch_id);

    void notify_event(const std::string& path, contracts::FileWatchEvent event);
    [[nodiscard]] size_t active_watchers_count() const noexcept;

private:
    struct WatchEntry {
        uint32_t watch_id;
        std::string path;
        FileWatchCallback callback;
    };

    mutable std::mutex mutex_;
    std::unordered_map<uint32_t, WatchEntry> watchers_;
    uint32_t next_watch_id_{1};
};

using FileWatcherPtr = std::shared_ptr<FileWatcher>;

} // namespace vani::capabilities::system
