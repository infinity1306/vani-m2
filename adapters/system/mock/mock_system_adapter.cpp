#include "mock_system_adapter.hpp"
#include <chrono>
#include <algorithm>

namespace vani::adapters::system {

MockSystemAdapter::MockSystemAdapter() {
    init_mock_data();
}

void MockSystemAdapter::init_mock_data() {
    // 1. Applications
    apps_["com.google.chrome"] = contracts::ApplicationMetadata{
        "com.google.chrome", "Google Chrome", {"chrome", "browser", "google-chrome"},
        "chrome.exe", "C:/Program Files/Google/Chrome/chrome.exe", "chrome.exe",
        "chrome.ico", "windows", false, {}, {}, "120.0.0", {"browser.open", "browser.navigate"}
    };

    apps_["com.microsoft.vscode"] = contracts::ApplicationMetadata{
        "com.microsoft.vscode", "Visual Studio Code", {"code", "vscode", "vs code"},
        "code.exe", "C:/Users/user/AppData/Local/Programs/Microsoft VS Code/Code.exe", "code.exe",
        "vscode.ico", "windows", false, {}, {}, "1.85.0", {"file.open", "terminal.open"}
    };

    apps_["com.spotify.client"] = contracts::ApplicationMetadata{
        "com.spotify.client", "Spotify", {"spotify", "music"},
        "spotify.exe", "C:/Users/user/AppData/Roaming/Spotify/Spotify.exe", "spotify.exe",
        "spotify.ico", "windows", false, {}, {}, "1.2.0", {"media.play", "media.pause"}
    };

    apps_["com.google.youtube"] = contracts::ApplicationMetadata{
        "com.google.youtube", "YouTube", {"youtube", "yt", "videos", "media"},
        "chrome.exe", "C:/Program Files/Google/Chrome/chrome.exe", "chrome.exe",
        "youtube.ico", "windows", false, {}, {}, "1.0.0", {"browser.open"}
    };

    apps_["com.microsoft.terminal"] = contracts::ApplicationMetadata{
        "com.microsoft.terminal", "Windows Terminal", {"terminal", "wt", "shell", "powershell"},
        "wt.exe", "C:/Program Files/WindowsApps/wt.exe", "wt.exe",
        "terminal.ico", "windows", false, {}, {}, "1.18.0", {"terminal.execute"}
    };

    apps_["com.github.desktop"] = contracts::ApplicationMetadata{
        "com.github.desktop", "GitHub Desktop", {"github", "git desktop"},
        "githubdesktop.exe", "C:/Users/User/AppData/Local/GitHubDesktop/GitHubDesktop.exe", "githubdesktop.exe",
        "github.ico", "windows", false, {}, {}, "3.3.0", {"git.sync"}
    };

    // 2. Processes
    processes_[100] = contracts::ProcessMetadata{
        100, "vani-runtime", "C:/vani/vani-runtime.exe", 1, 1.5, 45 * 1024 * 1024,
        1700000000000, contracts::ProcessStatus::Running, "system_user", {"vani-runtime"}
    };
    processes_[101] = contracts::ProcessMetadata{
        101, "explorer.exe", "C:/Windows/explorer.exe", 1, 0.5, 120 * 1024 * 1024,
        1700000000000, contracts::ProcessStatus::Running, "system_user", {"explorer.exe"}
    };

    // 3. Filesystem Mock
    files_["/workspace/vani/README.md"] = "# VANI Mark 2\nLocal-First AI Operating Layer";
    files_["/workspace/vani/config.json"] = "{\"version\": \"2.0.0\"}";
    files_["/workspace/vani/src/main.cpp"] = "#include <iostream>\nint main() { return 0; }";

    // 4. Windows
    windows_[1] = contracts::WindowMetadata{
        1, "com.microsoft.vscode", "main.cpp - VANI Mark 2",
        {100, 100, 1200, 800}, 0, contracts::WindowState::Normal, true
    };
    windows_[2] = contracts::WindowMetadata{
        2, "com.google.chrome", "New Tab - Google Chrome",
        {150, 150, 1024, 768}, 0, contracts::WindowState::Normal, false
    };

    // 5. Displays
    displays_.push_back(contracts::DisplayMetadata{0, "Primary Display", 1920, 1080, 60, 100, true});
    displays_.push_back(contracts::DisplayMetadata{1, "Secondary Display", 2560, 1440, 144, 80, false});

    // 6. Media Status
    media_status_ = contracts::MediaStatus{
        contracts::MediaPlaybackState::Playing, "Starboy", "The Weeknd", 50, false
    };

    // 7. System State
    system_state_ = contracts::SystemStateSnapshot{
        18.5, 8ULL * 1024 * 1024 * 1024, 16ULL * 1024 * 1024 * 1024,
        12.0, 2ULL * 1024 * 1024 * 1024, 8ULL * 1024 * 1024 * 1024,
        250ULL * 1024 * 1024 * 1024, 512ULL * 1024 * 1024 * 1024,
        85, true, true, "main.cpp - VANI Mark 2", 14400, "Windows", "11 Pro", "vani_user", 2
    };

    // 8. Clipboard
    clipboard_ = contracts::ClipboardPayload{
        contracts::ClipboardContentType::Text, "Initial mock clipboard content", {}, false
    };
}

contracts::PlatformId MockSystemAdapter::platform_id() const noexcept {
    return contracts::PlatformId::Universal;
}

std::string MockSystemAdapter::platform_name() const noexcept {
    return "MockUniversalPlatform";
}

contracts::Result<contracts::ApplicationMetadata> MockSystemAdapter::launch_application(
    const std::string& app_identifier,
    const std::vector<std::string>& /*args*/
) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [id, meta] : apps_) {
        bool match = (id == app_identifier || meta.name == app_identifier);
        if (!match) {
            for (const auto& alias : meta.aliases) {
                if (alias == app_identifier) {
                    match = true;
                    break;
                }
            }
        }

        if (match) {
            meta.is_running = true;
            uint32_t new_pid = next_pid_++;
            meta.process_ids.push_back(new_pid);
            processes_[new_pid] = contracts::ProcessMetadata{
                new_pid, meta.executable, meta.path, 1, 0.0, 50 * 1024 * 1024,
                1700000000000, contracts::ProcessStatus::Running, "mock_user", {meta.executable}
            };
            return contracts::Result<contracts::ApplicationMetadata>::success(meta);
        }
    }
    return contracts::Result<contracts::ApplicationMetadata>::failure(
        contracts::ErrorCode::NotFound, "Application not found: " + app_identifier
    );
}

contracts::Result<void> MockSystemAdapter::terminate_application(
    const std::string& app_identifier,
    bool /*force*/
) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [id, meta] : apps_) {
        bool match = (id == app_identifier || meta.name == app_identifier);
        if (!match) {
            for (const auto& alias : meta.aliases) {
                if (alias == app_identifier) {
                    match = true;
                    break;
                }
            }
        }

        if (match) {
            meta.is_running = false;
            for (uint32_t pid : meta.process_ids) {
                processes_.erase(pid);
            }
            meta.process_ids.clear();
            return contracts::Result<void>::success();
        }
    }
    return contracts::Result<void>::failure(
        contracts::ErrorCode::NotFound, "Application not found: " + app_identifier
    );
}

contracts::Result<void> MockSystemAdapter::focus_application(const std::string& app_identifier) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [id, meta] : apps_) {
        if (id == app_identifier || meta.name == app_identifier) {
            return contracts::Result<void>::success();
        }
    }
    return contracts::Result<void>::failure(
        contracts::ErrorCode::NotFound, "Application not found: " + app_identifier
    );
}

contracts::Result<std::vector<contracts::ApplicationMetadata>> MockSystemAdapter::list_installed_applications() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<contracts::ApplicationMetadata> list;
    list.reserve(apps_.size());
    for (const auto& [id, meta] : apps_) {
        list.push_back(meta);
    }
    return contracts::Result<std::vector<contracts::ApplicationMetadata>>::success(list);
}

contracts::Result<std::vector<contracts::ApplicationMetadata>> MockSystemAdapter::list_running_applications() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<contracts::ApplicationMetadata> list;
    for (const auto& [id, meta] : apps_) {
        if (meta.is_running) {
            list.push_back(meta);
        }
    }
    return contracts::Result<std::vector<contracts::ApplicationMetadata>>::success(list);
}

contracts::Result<std::vector<contracts::ProcessMetadata>> MockSystemAdapter::list_processes() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<contracts::ProcessMetadata> list;
    list.reserve(processes_.size());
    for (const auto& [pid, meta] : processes_) {
        list.push_back(meta);
    }
    return contracts::Result<std::vector<contracts::ProcessMetadata>>::success(list);
}

contracts::Result<contracts::ProcessMetadata> MockSystemAdapter::inspect_process(uint32_t pid) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(pid);
    if (it != processes_.end()) {
        return contracts::Result<contracts::ProcessMetadata>::success(it->second);
    }
    return contracts::Result<contracts::ProcessMetadata>::failure(
        contracts::ErrorCode::NotFound, "Process not found: " + std::to_string(pid)
    );
}

contracts::Result<uint32_t> MockSystemAdapter::start_process(
    const std::string& command,
    const std::string& /*working_dir*/,
    const std::unordered_map<std::string, std::string>& /*env*/
) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint32_t new_pid = next_pid_++;
    processes_[new_pid] = contracts::ProcessMetadata{
        new_pid, command, "/bin/" + command, 1, 0.1, 10 * 1024 * 1024,
        1700000000000, contracts::ProcessStatus::Running, "mock_user", {command}
    };
    return contracts::Result<uint32_t>::success(new_pid);
}

contracts::Result<void> MockSystemAdapter::stop_process(uint32_t pid, bool /*force*/) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(pid);
    if (it != processes_.end()) {
        processes_.erase(it);
        return contracts::Result<void>::success();
    }
    return contracts::Result<void>::failure(
        contracts::ErrorCode::NotFound, "Process not found: " + std::to_string(pid)
    );
}

contracts::Result<std::string> MockSystemAdapter::read_file(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = files_.find(path);
    if (it != files_.end()) {
        return contracts::Result<std::string>::success(it->second);
    }
    return contracts::Result<std::string>::failure(
        contracts::ErrorCode::NotFound, "File not found: " + path
    );
}

contracts::Result<void> MockSystemAdapter::write_file(const std::string& path, const std::string& content) {
    std::lock_guard<std::mutex> lock(mutex_);
    files_[path] = content;
    return contracts::Result<void>::success();
}

contracts::Result<void> MockSystemAdapter::create_directory(const std::string& /*path*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> MockSystemAdapter::move_file(const std::string& source, const std::string& dest) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = files_.find(source);
    if (it != files_.end()) {
        files_[dest] = it->second;
        files_.erase(it);
        return contracts::Result<void>::success();
    }
    return contracts::Result<void>::failure(
        contracts::ErrorCode::NotFound, "Source file not found: " + source
    );
}

contracts::Result<void> MockSystemAdapter::copy_file(const std::string& source, const std::string& dest) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = files_.find(source);
    if (it != files_.end()) {
        files_[dest] = it->second;
        return contracts::Result<void>::success();
    }
    return contracts::Result<void>::failure(
        contracts::ErrorCode::NotFound, "Source file not found: " + source
    );
}

contracts::Result<void> MockSystemAdapter::delete_file(const std::string& path, bool use_trash) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = files_.find(path);
    if (it != files_.end()) {
        if (use_trash) {
            trash_bin_.push_back(path);
        }
        files_.erase(it);
        return contracts::Result<void>::success();
    }
    return contracts::Result<void>::failure(
        contracts::ErrorCode::NotFound, "File not found: " + path
    );
}

contracts::Result<contracts::FileMetadata> MockSystemAdapter::get_file_metadata(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = files_.find(path);
    if (it != files_.end()) {
        contracts::FileMetadata meta;
        meta.path = path;
        meta.name = path.substr(path.find_last_of('/') + 1);
        meta.size_bytes = it->second.size();
        meta.type = contracts::FileType::Regular;
        meta.created_at_ms = 1700000000000;
        meta.modified_at_ms = 1700000000000;
        return contracts::Result<contracts::FileMetadata>::success(meta);
    }
    return contracts::Result<contracts::FileMetadata>::failure(
        contracts::ErrorCode::NotFound, "File not found: " + path
    );
}

contracts::Result<std::vector<contracts::FileMetadata>> MockSystemAdapter::search_files(
    const std::string& root_path,
    const std::string& pattern
) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<contracts::FileMetadata> results;
    for (const auto& [p, content] : files_) {
        if (p.rfind(root_path, 0) == 0 && (pattern.empty() || p.find(pattern) != std::string::npos)) {
            contracts::FileMetadata meta;
            meta.path = p;
            meta.name = p.substr(p.find_last_of('/') + 1);
            meta.size_bytes = content.size();
            meta.type = contracts::FileType::Regular;
            results.push_back(meta);
        }
    }
    return contracts::Result<std::vector<contracts::FileMetadata>>::success(results);
}

contracts::Result<contracts::TerminalExecutionResult> MockSystemAdapter::execute_command(
    const contracts::TerminalExecutionRequest& request,
    const contracts::CancellationToken& cancel_token
) {
    if (cancel_token.is_cancelled()) {
        contracts::TerminalExecutionResult res;
        res.command = request.command;
        res.cwd = request.working_directory;
        res.cancelled = true;
        res.exit_code = 130;
        return contracts::Result<contracts::TerminalExecutionResult>::success(res);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    contracts::TerminalExecutionResult res;
    res.command = request.command;
    res.cwd = request.working_directory.empty() ? "/workspace" : request.working_directory;
    res.exit_code = sim_exit_code_;
    res.stdout_content = sim_stdout_;
    res.stderr_content = sim_stderr_;
    res.duration_ms = 15;
    res.process_id = 4242;
    res.timed_out = false;
    res.cancelled = false;
    res.output_truncated = false;

    return contracts::Result<contracts::TerminalExecutionResult>::success(res);
}

contracts::Result<std::vector<contracts::WindowMetadata>> MockSystemAdapter::list_windows() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<contracts::WindowMetadata> list;
    list.reserve(windows_.size());
    for (const auto& [id, meta] : windows_) {
        list.push_back(meta);
    }
    return contracts::Result<std::vector<contracts::WindowMetadata>>::success(list);
}

contracts::Result<void> MockSystemAdapter::focus_window(uint32_t window_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = windows_.find(window_id);
    if (it != windows_.end()) {
        for (auto& [id, win] : windows_) {
            win.is_focused = (id == window_id);
        }
        return contracts::Result<void>::success();
    }
    return contracts::Result<void>::failure(
        contracts::ErrorCode::NotFound, "Window not found: " + std::to_string(window_id)
    );
}

contracts::Result<void> MockSystemAdapter::set_window_state(uint32_t window_id, contracts::WindowState state) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = windows_.find(window_id);
    if (it != windows_.end()) {
        it->second.state = state;
        return contracts::Result<void>::success();
    }
    return contracts::Result<void>::failure(
        contracts::ErrorCode::NotFound, "Window not found: " + std::to_string(window_id)
    );
}

contracts::Result<void> MockSystemAdapter::resize_window(uint32_t window_id, const contracts::WindowRect& rect) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = windows_.find(window_id);
    if (it != windows_.end()) {
        it->second.rect = rect;
        return contracts::Result<void>::success();
    }
    return contracts::Result<void>::failure(
        contracts::ErrorCode::NotFound, "Window not found: " + std::to_string(window_id)
    );
}

contracts::Result<void> MockSystemAdapter::close_window(uint32_t window_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = windows_.find(window_id);
    if (it != windows_.end()) {
        windows_.erase(it);
        return contracts::Result<void>::success();
    }
    return contracts::Result<void>::failure(
        contracts::ErrorCode::NotFound, "Window not found: " + std::to_string(window_id)
    );
}

contracts::Result<void> MockSystemAdapter::inject_key_press(
    const std::string& key,
    const contracts::InputTargetContext& /*target*/
) {
    std::lock_guard<std::mutex> lock(mutex_);
    input_history_.push_back("key_press:" + key);
    return contracts::Result<void>::success();
}

contracts::Result<void> MockSystemAdapter::inject_key_sequence(
    const std::string& text,
    const contracts::InputTargetContext& /*target*/
) {
    std::lock_guard<std::mutex> lock(mutex_);
    input_history_.push_back("key_sequence:" + text);
    return contracts::Result<void>::success();
}

contracts::Result<void> MockSystemAdapter::inject_mouse_click(
    contracts::MouseButton button,
    int32_t x,
    int32_t y,
    const contracts::InputTargetContext& /*target*/
) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string b = (button == contracts::MouseButton::Left) ? "left" : "right";
    input_history_.push_back("mouse_click:" + b + "@" + std::to_string(x) + "," + std::to_string(y));
    return contracts::Result<void>::success();
}

contracts::Result<void> MockSystemAdapter::inject_mouse_move(
    int32_t x,
    int32_t y,
    const contracts::InputTargetContext& /*target*/
) {
    std::lock_guard<std::mutex> lock(mutex_);
    input_history_.push_back("mouse_move@" + std::to_string(x) + "," + std::to_string(y));
    return contracts::Result<void>::success();
}

contracts::Result<void> MockSystemAdapter::inject_mouse_scroll(
    int32_t delta_y,
    const contracts::InputTargetContext& /*target*/
) {
    std::lock_guard<std::mutex> lock(mutex_);
    input_history_.push_back("mouse_scroll:" + std::to_string(delta_y));
    return contracts::Result<void>::success();
}

contracts::Result<contracts::ClipboardPayload> MockSystemAdapter::read_clipboard() {
    std::lock_guard<std::mutex> lock(mutex_);
    return contracts::Result<contracts::ClipboardPayload>::success(clipboard_);
}

contracts::Result<void> MockSystemAdapter::write_clipboard(const contracts::ClipboardPayload& payload) {
    std::lock_guard<std::mutex> lock(mutex_);
    clipboard_ = payload;
    return contracts::Result<void>::success();
}

contracts::Result<void> MockSystemAdapter::clear_clipboard() {
    std::lock_guard<std::mutex> lock(mutex_);
    clipboard_ = contracts::ClipboardPayload{contracts::ClipboardContentType::Empty, "", {}, false};
    return contracts::Result<void>::success();
}

contracts::Result<contracts::ScreenCaptureMetadata> MockSystemAdapter::capture_screen(
    uint32_t monitor_id,
    uint32_t window_id
) {
    contracts::ScreenCaptureMetadata cap;
    cap.capture_id = "cap_mock_001";
    cap.timestamp_ms = 1700000000000;
    cap.monitor_id = monitor_id;
    cap.window_id = window_id;
    cap.width = 1920;
    cap.height = 1080;
    cap.format = "png";
    cap.source = "mock_screen";
    cap.image_data = {0x89, 0x50, 0x4E, 0x47}; // PNG magic bytes
    return contracts::Result<contracts::ScreenCaptureMetadata>::success(cap);
}

contracts::Result<std::vector<contracts::DisplayMetadata>> MockSystemAdapter::list_displays() {
    std::lock_guard<std::mutex> lock(mutex_);
    return contracts::Result<std::vector<contracts::DisplayMetadata>>::success(displays_);
}

contracts::Result<uint32_t> MockSystemAdapter::get_brightness(uint32_t display_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& d : displays_) {
        if (d.display_id == display_id) {
            return contracts::Result<uint32_t>::success(d.brightness_percent);
        }
    }
    return contracts::Result<uint32_t>::failure(contracts::ErrorCode::NotFound, "Display not found");
}

contracts::Result<void> MockSystemAdapter::set_brightness(uint32_t display_id, uint32_t percent) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& d : displays_) {
        if (d.display_id == display_id) {
            d.brightness_percent = std::min(100u, percent);
            return contracts::Result<void>::success();
        }
    }
    return contracts::Result<void>::failure(contracts::ErrorCode::NotFound, "Display not found");
}

contracts::Result<contracts::MediaStatus> MockSystemAdapter::get_media_status() {
    std::lock_guard<std::mutex> lock(mutex_);
    return contracts::Result<contracts::MediaStatus>::success(media_status_);
}

contracts::Result<void> MockSystemAdapter::media_play() {
    std::lock_guard<std::mutex> lock(mutex_);
    media_status_.state = contracts::MediaPlaybackState::Playing;
    return contracts::Result<void>::success();
}

contracts::Result<void> MockSystemAdapter::media_pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    media_status_.state = contracts::MediaPlaybackState::Paused;
    return contracts::Result<void>::success();
}

contracts::Result<void> MockSystemAdapter::media_stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    media_status_.state = contracts::MediaPlaybackState::Stopped;
    return contracts::Result<void>::success();
}

contracts::Result<void> MockSystemAdapter::media_next() {
    std::lock_guard<std::mutex> lock(mutex_);
    media_status_.current_track = "Next Track (Simulated)";
    return contracts::Result<void>::success();
}

contracts::Result<void> MockSystemAdapter::media_previous() {
    std::lock_guard<std::mutex> lock(mutex_);
    media_status_.current_track = "Previous Track (Simulated)";
    return contracts::Result<void>::success();
}

contracts::Result<void> MockSystemAdapter::set_volume(uint32_t volume_percent) {
    std::lock_guard<std::mutex> lock(mutex_);
    media_status_.volume_percent = std::min(100u, volume_percent);
    return contracts::Result<void>::success();
}

contracts::Result<contracts::SystemStateSnapshot> MockSystemAdapter::get_system_state() {
    std::lock_guard<std::mutex> lock(mutex_);
    return contracts::Result<contracts::SystemStateSnapshot>::success(system_state_);
}

contracts::Result<void> MockSystemAdapter::execute_power_action(contracts::PowerAction action) {
    std::lock_guard<std::mutex> lock(mutex_);
    executed_power_actions_.push_back(action);
    return contracts::Result<void>::success();
}

contracts::Result<void> MockSystemAdapter::send_notification(const contracts::NotificationPayload& notification) {
    std::lock_guard<std::mutex> lock(mutex_);
    sent_notifications_.push_back(notification);
    return contracts::Result<void>::success();
}

const std::vector<contracts::NotificationPayload>& MockSystemAdapter::sent_notifications() const noexcept {
    return sent_notifications_;
}

const std::vector<contracts::PowerAction>& MockSystemAdapter::executed_power_actions() const noexcept {
    return executed_power_actions_;
}

const std::vector<std::string>& MockSystemAdapter::input_history() const noexcept {
    return input_history_;
}

const std::vector<std::string>& MockSystemAdapter::trash_bin() const noexcept {
    return trash_bin_;
}

void MockSystemAdapter::set_simulated_exit_code(int32_t code) {
    sim_exit_code_ = code;
}

void MockSystemAdapter::set_simulated_stdout(const std::string& out) {
    sim_stdout_ = out;
}

void MockSystemAdapter::set_simulated_stderr(const std::string& err) {
    sim_stderr_ = err;
}

} // namespace vani::adapters::system
