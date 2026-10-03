#include "browser_manager.hpp"
#include "../../../adapters/network/http_client.hpp"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

#include <regex>
#include <sstream>
#include <cstring>

namespace vani::capabilities::system {

static std::string strip_html_tags(const std::string& html) {
    try {
        // 1. Remove <script>...</script>
        std::string no_script = std::regex_replace(html, std::regex("<script[\\s\\S]*?</script>", std::regex::icase), " ");
        // 2. Remove <style>...</style>
        std::string no_style = std::regex_replace(no_script, std::regex("<style[\\s\\S]*?</style>", std::regex::icase), " ");
        // 3. Remove all HTML tags
        std::string text_only = std::regex_replace(no_style, std::regex("<[^>]+>"), " ");
        // 4. Collapse whitespace
        std::string clean;
        clean.reserve(text_only.size());
        bool in_space = false;
        for (char c : text_only) {
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                if (!in_space) {
                    clean += ' ';
                    in_space = true;
                }
            } else {
                clean += c;
                in_space = false;
            }
        }
        return clean;
    } catch (...) {
        return html;
    }
}

BrowserManager::BrowserManager() = default;

contracts::Result<BrowserSession> BrowserManager::open_browser(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string s_id = "browser_sess_" + std::to_string(next_session_seq_++);
    BrowserSession session;
    session.session_id = s_id;
    session.task_id = task_id;
    session.is_open = true;

    BrowserTab initial_tab{"tab_1", "https://www.google.com", "Google Search", true};
    session.tabs.push_back(initial_tab);
    session.active_tab_id = "tab_1";

#ifdef _WIN32
    ShellExecuteA(NULL, "open", "https://www.google.com", NULL, NULL, SW_SHOWNORMAL);
#endif

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

#ifdef _WIN32
    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif

    return contracts::Result<void>::success();
}

contracts::Result<void> BrowserManager::search(const std::string& session_id, const std::string& query) {
    std::string encoded_query;
    for (char c : query) {
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded_query += c;
        } else if (c == ' ') {
            encoded_query += '+';
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned char>(c));
            encoded_query += buf;
        }
    }
    return navigate(session_id, "https://www.google.com/search?q=" + encoded_query);
}

contracts::Result<std::string> BrowserManager::get_page_content(const std::string& session_id) {
    std::string active_url;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(session_id);
        if (it == sessions_.end() || !it->second.is_open) {
            return contracts::Result<std::string>::failure(
                contracts::ErrorCode::NotFound, "Active browser session not found: " + session_id
            );
        }

        for (const auto& tab : it->second.tabs) {
            if (tab.tab_id == it->second.active_tab_id) {
                active_url = tab.url;
                break;
            }
        }
    }

    if (active_url.empty() || active_url == "about:blank") {
        return contracts::Result<std::string>::failure(
            contracts::ErrorCode::ValidationError, "Active tab has no valid URL loaded"
        );
    }

    if (!active_url.starts_with("http://") && !active_url.starts_with("https://")) {
        return contracts::Result<std::string>::failure(
            contracts::ErrorCode::ValidationError, "Cannot fetch non-HTTP page content: " + active_url
        );
    }

    // Fetch live HTML via WinHTTP
    auto fetch_res = adapters::network::HttpClient::get(active_url, 10000);
    if (!fetch_res.is_success()) {
        return contracts::Result<std::string>::failure(fetch_res.error());
    }

    return contracts::Result<std::string>::success(fetch_res.value().body);
}

contracts::Result<std::string> BrowserManager::extract_text(const std::string& session_id, const std::string& /*selector*/) {
    auto content_res = get_page_content(session_id);
    if (!content_res.is_success()) {
        return content_res;
    }

    std::string raw_html = content_res.value();
    std::string clean_text = strip_html_tags(raw_html);

    if (clean_text.empty()) {
        return contracts::Result<std::string>::failure(
            contracts::ErrorCode::InternalError, "Failed to extract text from page"
        );
    }

    return contracts::Result<std::string>::success(clean_text);
}

contracts::Result<std::vector<uint8_t>> BrowserManager::capture_screenshot(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end() || !it->second.is_open) {
        return contracts::Result<std::vector<uint8_t>>::failure(
            contracts::ErrorCode::NotFound, "Active browser session not found: " + session_id
        );
    }

    std::vector<uint8_t> screenshot_data;

#ifdef _WIN32
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    DWORD image_size = ((width * 24 + 31) / 32) * 4 * height;
    screenshot_data.resize(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + image_size);

    BITMAPFILEHEADER bfh = {0};
    bfh.bfType = 0x4D42; // "BM"
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + image_size;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    std::memcpy(screenshot_data.data(), &bfh, sizeof(BITMAPFILEHEADER));
    std::memcpy(screenshot_data.data() + sizeof(BITMAPFILEHEADER), &bmi.bmiHeader, sizeof(BITMAPINFOHEADER));

    GetDIBits(hMemoryDC, hBitmap, 0, height, screenshot_data.data() + bfh.bfOffBits, &bmi, DIB_RGB_COLORS);

    SelectObject(hMemoryDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);
#endif

    if (screenshot_data.empty()) {
        return contracts::Result<std::vector<uint8_t>>::failure(
            contracts::ErrorCode::InternalError, "Failed to capture browser screenshot"
        );
    }

    return contracts::Result<std::vector<uint8_t>>::success(screenshot_data);
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
