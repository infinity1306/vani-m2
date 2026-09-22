#include "browser_manager.hpp"

namespace vani::capabilities::system {

BrowserManager::BrowserManager() = default;

contracts::Result<BrowserSession> BrowserManager::open_browser(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string s_id = "browser_sess_" + std::to_string(next_session_seq_++);
    BrowserSession session;
    session.session_id = s_id;
    session.task_id = task_id;
    session.is_open = true;

    BrowserTab initial_tab{"tab_1", "about:blank", "New Tab", true};
    session.tabs.push_back(initial_tab);
    session.active_tab_id = "tab_1";

    sessions_[s_id] = session;
    return contracts::Result<BrowserSession>::success(session);
}

contracts::Result<void> BrowserManager::navigate(const std::string& session_id, const std::string& url) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end() || !it->second.is_open) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::NotFound, "Active browser session not found: " + session_id
        );
    }

    for (auto& tab : it->second.tabs) {
        if (tab.tab_id == it->second.active_tab_id) {
            tab.url = url;
            tab.title = "Page: " + url;
            break;
        }
    }
    return contracts::Result<void>::success();
}

contracts::Result<void> BrowserManager::search(const std::string& session_id, const std::string& query) {
    return navigate(session_id, "https://www.google.com/search?q=" + query);
}

contracts::Result<std::string> BrowserManager::get_page_content(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end() || !it->second.is_open) {
        return contracts::Result<std::string>::failure(
            contracts::ErrorCode::NotFound, "Active browser session not found: " + session_id
        );
    }
    return contracts::Result<std::string>::success("<html><body><h1>Simulated Page</h1><p>Content</p></body></html>");
}

contracts::Result<std::string> BrowserManager::extract_text(const std::string& session_id, const std::string& /*selector*/) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end() || !it->second.is_open) {
        return contracts::Result<std::string>::failure(
            contracts::ErrorCode::NotFound, "Active browser session not found: " + session_id
        );
    }
    return contracts::Result<std::string>::success("Extracted text from page");
}

contracts::Result<std::vector<uint8_t>> BrowserManager::capture_screenshot(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end() || !it->second.is_open) {
        return contracts::Result<std::vector<uint8_t>>::failure(
            contracts::ErrorCode::NotFound, "Active browser session not found: " + session_id
        );
    }
    return contracts::Result<std::vector<uint8_t>>::success({0x89, 0x50, 0x4E, 0x47});
}

contracts::Result<void> BrowserManager::click(const std::string& session_id, const std::string& /*selector*/) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end() || !it->second.is_open) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::NotFound, "Active browser session not found: " + session_id
        );
    }
    return contracts::Result<void>::success();
}

contracts::Result<void> BrowserManager::type_text(
    const std::string& session_id,
    const std::string& /*selector*/,
    const std::string& /*text*/
) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end() || !it->second.is_open) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::NotFound, "Active browser session not found: " + session_id
        );
    }
    return contracts::Result<void>::success();
}

contracts::Result<void> BrowserManager::select_option(
    const std::string& session_id,
    const std::string& /*selector*/,
    const std::string& /*value*/
) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end() || !it->second.is_open) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::NotFound, "Active browser session not found: " + session_id
        );
    }
    return contracts::Result<void>::success();
}

contracts::Result<void> BrowserManager::close_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        it->second.is_open = false;
        return contracts::Result<void>::success();
    }
    return contracts::Result<void>::failure(
        contracts::ErrorCode::NotFound, "Session not found: " + session_id
    );
}

std::optional<BrowserSession> BrowserManager::get_session(const std::string& session_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<BrowserSession> BrowserManager::list_sessions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<BrowserSession> list;
    list.reserve(sessions_.size());
    for (const auto& [id, s] : sessions_) {
        list.push_back(s);
    }
    return list;
}

} // namespace vani::capabilities::system
