#include "application_registry.hpp"
#include <algorithm>
#include <cctype>

namespace vani::capabilities::system {

ApplicationRegistry::ApplicationRegistry() {
    load_standard_registry();
}

std::string ApplicationRegistry::normalize(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    return result;
}

void ApplicationRegistry::register_application(const contracts::ApplicationMetadata& app) {
    std::lock_guard<std::mutex> lock(mutex_);
    apps_[app.application_id] = app;

    // Index canonical name & ID
    alias_to_app_id_[normalize(app.application_id)] = app.application_id;
    alias_to_app_id_[normalize(app.name)] = app.application_id;
    alias_to_app_id_[normalize(app.executable)] = app.application_id;

    // Index all aliases
    for (const auto& alias : app.aliases) {
        alias_to_app_id_[normalize(alias)] = app.application_id;
    }
}

void ApplicationRegistry::unregister_application(const std::string& application_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = apps_.find(application_id);
    if (it != apps_.end()) {
        const auto& app = it->second;
        alias_to_app_id_.erase(normalize(app.application_id));
        alias_to_app_id_.erase(normalize(app.name));
        alias_to_app_id_.erase(normalize(app.executable));
        for (const auto& alias : app.aliases) {
            alias_to_app_id_.erase(normalize(alias));
        }
        apps_.erase(it);
    }
}

std::optional<contracts::ApplicationMetadata> ApplicationRegistry::resolve(const std::string& name_or_alias) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = normalize(name_or_alias);
    auto it = alias_to_app_id_.find(key);
    if (it != alias_to_app_id_.end()) {
        auto app_it = apps_.find(it->second);
        if (app_it != apps_.end()) {
            return app_it->second;
        }
    }
    return std::nullopt;
}

std::vector<contracts::ApplicationMetadata> ApplicationRegistry::list_all() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<contracts::ApplicationMetadata> result;
    result.reserve(apps_.size());
    for (const auto& [id, app] : apps_) {
        result.push_back(app);
    }
    return result;
}

size_t ApplicationRegistry::count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return apps_.size();
}

void ApplicationRegistry::load_standard_registry() {
    register_application({
        "com.google.chrome", "Google Chrome", {"chrome", "google-chrome", "browser", "internet"},
        "chrome.exe", "C:/Program Files/Google/Chrome/Application/chrome.exe", "chrome.exe",
        "chrome.ico", "windows", false, {}, {}, "120.0.0", {"browser.open", "browser.navigate"}
    });

    register_application({
        "com.microsoft.vscode", "Visual Studio Code", {"vscode", "vs code", "code", "editor"},
        "code.exe", "C:/Users/User/AppData/Local/Programs/Microsoft VS Code/Code.exe", "code.exe",
        "code.ico", "windows", false, {}, {}, "1.85.0", {"file.open", "terminal.open"}
    });

    register_application({
        "com.spotify.client", "Spotify", {"spotify", "music", "songs"},
        "spotify.exe", "C:/Users/User/AppData/Roaming/Spotify/Spotify.exe", "spotify.exe",
        "spotify.ico", "windows", false, {}, {}, "1.2.0", {"media.play", "media.pause"}
    });

    register_application({
        "com.microsoft.terminal", "Windows Terminal", {"terminal", "powershell", "cmd", "bash", "shell"},
        "wt.exe", "C:/Program Files/WindowsApps/wt.exe", "wt.exe",
        "terminal.ico", "windows", false, {}, {}, "1.18.0", {"terminal.execute"}
    });

    register_application({
        "com.github.desktop", "GitHub Desktop", {"github", "git desktop"},
        "githubdesktop.exe", "C:/Users/User/AppData/Local/GitHubDesktop/GitHubDesktop.exe", "githubdesktop.exe",
        "github.ico", "windows", false, {}, {}, "3.3.0", {"git.sync"}
    });

    register_application({
        "com.google.youtube", "YouTube", {"youtube", "yt", "videos", "media"},
        "chrome.exe", "https://youtube.com", "chrome.exe",
        "youtube.ico", "windows", false, {}, {}, "1.0.0", {"browser.open"}
    });
}

} // namespace vani::capabilities::system
