#pragma once

#include "browser_session.hpp"
#include "../../../contracts/common/result.hpp"
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>

namespace vani::capabilities::system {

class BrowserManager {
public:
    BrowserManager();
    ~BrowserManager() = default;

    // Read-only actions
    contracts::Result<BrowserSession> open_browser(const std::string& task_id = "");
    contracts::Result<void> navigate(const std::string& session_id, const std::string& url);
    contracts::Result<void> search(const std::string& session_id, const std::string& query);
    contracts::Result<std::string> get_page_content(const std::string& session_id);
    contracts::Result<std::string> extract_text(const std::string& session_id, const std::string& selector);
    contracts::Result<std::vector<uint8_t>> capture_screenshot(const std::string& session_id);

    // State-changing actions
    contracts::Result<void> click(const std::string& session_id, const std::string& selector);
    contracts::Result<void> type_text(const std::string& session_id, const std::string& selector, const std::string& text);
    contracts::Result<void> select_option(const std::string& session_id, const std::string& selector, const std::string& value);
    contracts::Result<void> close_session(const std::string& session_id);

    [[nodiscard]] std::optional<BrowserSession> get_session(const std::string& session_id) const;
    [[nodiscard]] std::vector<BrowserSession> list_sessions() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, BrowserSession> sessions_;
    uint32_t next_session_seq_{1};
};

using BrowserManagerPtr = std::shared_ptr<BrowserManager>;

} // namespace vani::capabilities::system
