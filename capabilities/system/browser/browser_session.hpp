#pragma once

#include "../../../contracts/system/system_contracts.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace vani::capabilities::system {

enum class BrowserActionType : uint8_t {
    // Read-only
    Open,
    Navigate,
    Search,
    GetPage,
    Screenshot,
    Extract,
    // State-changing
    Click,
    Type,
    Select,
    SubmitForm
};

[[nodiscard]] constexpr bool is_state_changing_browser_action(BrowserActionType action) noexcept {
    switch (action) {
        case BrowserActionType::Click:
        case BrowserActionType::Type:
        case BrowserActionType::Select:
        case BrowserActionType::SubmitForm:
            return true;
        default:
            return false;
    }
}

struct BrowserTab {
    std::string tab_id;
    std::string url;
    std::string title;
    bool is_active{false};
};

struct BrowserSession {
    std::string session_id;
    std::string browser_name{"chromium"};
    std::string profile{"default"};
    std::vector<BrowserTab> tabs;
    std::string active_tab_id;
    std::string task_id;
    bool is_open{false};
};

} // namespace vani::capabilities::system
