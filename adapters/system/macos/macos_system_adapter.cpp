#include "macos_system_adapter.hpp"

namespace vani::adapters::system {

MacOSSystemAdapter::MacOSSystemAdapter() = default;

contracts::PlatformId MacOSSystemAdapter::platform_id() const noexcept {
    return contracts::PlatformId::MacOS;
}

std::string MacOSSystemAdapter::platform_name() const noexcept {
    return "macOS";
}

contracts::Result<contracts::ApplicationMetadata> MacOSSystemAdapter::launch_application(
    const std::string& app_identifier,
    const std::vector<std::string>& /*args*/
) {
    contracts::ApplicationMetadata meta;
    meta.application_id = app_identifier;
    meta.name = app_identifier;
    meta.executable = "/Applications/" + app_identifier + ".app";
    meta.is_running = true;
    meta.platform = "macos";
    return contracts::Result<contracts::ApplicationMetadata>::success(meta);
}

contracts::Result<void> MacOSSystemAdapter::terminate_application(
    const std::string& /*app_identifier*/,
    bool /*force*/
) {
    return contracts::Result<void>::success();
}

contracts::Result<void> MacOSSystemAdapter::focus_application(const std::string& /*app_identifier*/) {
    return contracts::Result<void>::success();
}

contracts::Result<std::vector<contracts::ApplicationMetadata>> MacOSSystemAdapter::list_installed_applications() {
    std::vector<contracts::ApplicationMetadata> list;
    list.push_back({"com.apple.Safari", "Safari", {"safari", "browser"}, "/Applications/Safari.app", "/Applications/Safari.app", "Safari", "", "macos", false, {}, {}, "17.0", {}});
    return contracts::Result<std::vector<contracts::ApplicationMetadata>>::success(list);
}

contracts::Result<std::vector<contracts::ApplicationMetadata>> MacOSSystemAdapter::list_running_applications() {
    return contracts::Result<std::vector<contracts::ApplicationMetadata>>::success({});
}

contracts::Result<std::vector<contracts::ProcessMetadata>> MacOSSystemAdapter::list_processes() {
    std::vector<contracts::ProcessMetadata> procs;
    procs.push_back({1, "launchd", "/sbin/launchd", 0, 0.0, 20 * 1024 * 1024, 0, contracts::ProcessStatus::Running, "root", {"/sbin/launchd"}});
    return contracts::Result<std::vector<contracts::ProcessMetadata>>::success(procs);
}

contracts::Result<contracts::ProcessMetadata> MacOSSystemAdapter::inspect_process(uint32_t pid) {
    contracts::ProcessMetadata meta;
    meta.pid = pid;
    meta.name = "process_" + std::to_string(pid);
    meta.status = contracts::ProcessStatus::Running;
    return contracts::Result<contracts::ProcessMetadata>::success(meta);
}

contracts::Result<uint32_t> MacOSSystemAdapter::start_process(
    const std::string& /*command*/,
    const std::string& /*working_dir*/,
    const std::unordered_map<std::string, std::string>& /*env*/
) {
    return contracts::Result<uint32_t>::success(1024);
}

contracts::Result<void> MacOSSystemAdapter::stop_process(uint32_t /*pid*/, bool /*force*/) {
    return contracts::Result<void>::success();
}

contracts::Result<std::string> MacOSSystemAdapter::read_file(const std::string& /*path*/) {
    return contracts::Result<std::string>::success("macos file content");
}

contracts::Result<void> MacOSSystemAdapter::write_file(const std::string& /*path*/, const std::string& /*content*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> MacOSSystemAdapter::create_directory(const std::string& /*path*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> MacOSSystemAdapter::move_file(const std::string& /*source*/, const std::string& /*dest*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> MacOSSystemAdapter::copy_file(const std::string& /*source*/, const std::string& /*dest*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> MacOSSystemAdapter::delete_file(const std::string& /*path*/, bool /*use_trash*/) {
    return contracts::Result<void>::success();
}

contracts::Result<contracts::FileMetadata> MacOSSystemAdapter::get_file_metadata(const std::string& path) {
    contracts::FileMetadata meta;
    meta.path = path;
    meta.name = "file.txt";
    meta.size_bytes = 1024;
    meta.type = contracts::FileType::Regular;
    return contracts::Result<contracts::FileMetadata>::success(meta);
}

contracts::Result<std::vector<contracts::FileMetadata>> MacOSSystemAdapter::search_files(
    const std::string& /*root_path*/,
    const std::string& /*pattern*/
) {
    return contracts::Result<std::vector<contracts::FileMetadata>>::success({});
}

contracts::Result<contracts::TerminalExecutionResult> MacOSSystemAdapter::execute_command(
    const contracts::TerminalExecutionRequest& request,
    const contracts::CancellationToken& cancel_token
) {
    if (cancel_token.is_cancelled()) {
        contracts::TerminalExecutionResult res;
        res.command = request.command;
        res.cancelled = true;
        res.exit_code = 130;
        return contracts::Result<contracts::TerminalExecutionResult>::success(res);
    }
    contracts::TerminalExecutionResult res;
    res.command = request.command;
    res.cwd = request.working_directory;
    res.exit_code = 0;
    res.stdout_content = "macOS Command Executed";
    res.duration_ms = 10;
    res.process_id = 777;
    return contracts::Result<contracts::TerminalExecutionResult>::success(res);
}

contracts::Result<std::vector<contracts::WindowMetadata>> MacOSSystemAdapter::list_windows() {
    return contracts::Result<std::vector<contracts::WindowMetadata>>::success({});
}

contracts::Result<void> MacOSSystemAdapter::focus_window(uint32_t /*window_id*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> MacOSSystemAdapter::set_window_state(uint32_t /*window_id*/, contracts::WindowState /*state*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> MacOSSystemAdapter::resize_window(uint32_t /*window_id*/, const contracts::WindowRect& /*rect*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> MacOSSystemAdapter::close_window(uint32_t /*window_id*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> MacOSSystemAdapter::inject_key_press(
    const std::string& /*key*/,
    const contracts::InputTargetContext& /*target*/
) {
    return contracts::Result<void>::success();
}

contracts::Result<void> MacOSSystemAdapter::inject_key_sequence(
    const std::string& /*text*/,
    const contracts::InputTargetContext& /*target*/
) {
    return contracts::Result<void>::success();
}

contracts::Result<void> MacOSSystemAdapter::inject_mouse_click(
    contracts::MouseButton /*button*/,
    int32_t /*x*/,
    int32_t /*y*/,
    const contracts::InputTargetContext& /*target*/
) {
    return contracts::Result<void>::success();
}

contracts::Result<void> MacOSSystemAdapter::inject_mouse_move(
    int32_t /*x*/,
    int32_t /*y*/,
    const contracts::InputTargetContext& /*target*/
) {
    return contracts::Result<void>::success();
}

contracts::Result<void> MacOSSystemAdapter::inject_mouse_scroll(
    int32_t /*delta_y*/,
    const contracts::InputTargetContext& /*target*/
) {
    return contracts::Result<void>::success();
}

contracts::Result<contracts::ClipboardPayload> MacOSSystemAdapter::read_clipboard() {
    contracts::ClipboardPayload payload;
    payload.type = contracts::ClipboardContentType::Text;
    payload.text_content = "macOS clipboard text";
    return contracts::Result<contracts::ClipboardPayload>::success(payload);
}

contracts::Result<void> MacOSSystemAdapter::write_clipboard(const contracts::ClipboardPayload& /*payload*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> MacOSSystemAdapter::clear_clipboard() {
    return contracts::Result<void>::success();
}

contracts::Result<contracts::ScreenCaptureMetadata> MacOSSystemAdapter::capture_screen(
    uint32_t monitor_id,
    uint32_t window_id
) {
    contracts::ScreenCaptureMetadata meta;
    meta.capture_id = "mac_cap_001";
    meta.monitor_id = monitor_id;
    meta.window_id = window_id;
    meta.width = 1920;
    meta.height = 1080;
    meta.format = "png";
    meta.source = "core_graphics_capture";
    return contracts::Result<contracts::ScreenCaptureMetadata>::success(meta);
}

contracts::Result<std::vector<contracts::DisplayMetadata>> MacOSSystemAdapter::list_displays() {
    std::vector<contracts::DisplayMetadata> list;
    list.push_back({0, "Built-in Retina Display", 2880, 1800, 60, 100, true});
    return contracts::Result<std::vector<contracts::DisplayMetadata>>::success(list);
}

contracts::Result<uint32_t> MacOSSystemAdapter::get_brightness(uint32_t /*display_id*/) {
    return contracts::Result<uint32_t>::success(100);
}

contracts::Result<void> MacOSSystemAdapter::set_brightness(uint32_t /*display_id*/, uint32_t /*percent*/) {
    return contracts::Result<void>::success();
}

contracts::Result<contracts::MediaStatus> MacOSSystemAdapter::get_media_status() {
    contracts::MediaStatus s;
    s.state = contracts::MediaPlaybackState::Stopped;
    s.volume_percent = 50;
    return contracts::Result<contracts::MediaStatus>::success(s);
}

contracts::Result<void> MacOSSystemAdapter::media_play() { return contracts::Result<void>::success(); }
contracts::Result<void> MacOSSystemAdapter::media_pause() { return contracts::Result<void>::success(); }
contracts::Result<void> MacOSSystemAdapter::media_stop() { return contracts::Result<void>::success(); }
contracts::Result<void> MacOSSystemAdapter::media_next() { return contracts::Result<void>::success(); }
contracts::Result<void> MacOSSystemAdapter::media_previous() { return contracts::Result<void>::success(); }
contracts::Result<void> MacOSSystemAdapter::set_volume(uint32_t /*volume_percent*/) { return contracts::Result<void>::success(); }

contracts::Result<contracts::SystemStateSnapshot> MacOSSystemAdapter::get_system_state() {
    contracts::SystemStateSnapshot s;
    s.os_name = "macOS";
    s.os_version = "14.2";
    s.logged_in_user = "user";
    s.is_online = true;
    return contracts::Result<contracts::SystemStateSnapshot>::success(s);
}

contracts::Result<void> MacOSSystemAdapter::execute_power_action(contracts::PowerAction /*action*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> MacOSSystemAdapter::send_notification(const contracts::NotificationPayload& /*notification*/) {
    return contracts::Result<void>::success();
}

} // namespace vani::adapters::system
