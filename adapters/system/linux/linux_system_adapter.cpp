#include "linux_system_adapter.hpp"

namespace vani::adapters::system {

LinuxSystemAdapter::LinuxSystemAdapter() = default;

contracts::PlatformId LinuxSystemAdapter::platform_id() const noexcept {
    return contracts::PlatformId::Linux;
}

std::string LinuxSystemAdapter::platform_name() const noexcept {
    return "Linux";
}

contracts::Result<contracts::ApplicationMetadata> LinuxSystemAdapter::launch_application(
    const std::string& app_identifier,
    const std::vector<std::string>& /*args*/
) {
    contracts::ApplicationMetadata meta;
    meta.application_id = app_identifier;
    meta.name = app_identifier;
    meta.executable = "/usr/bin/" + app_identifier;
    meta.is_running = true;
    meta.platform = "linux";
    return contracts::Result<contracts::ApplicationMetadata>::success(meta);
}

contracts::Result<void> LinuxSystemAdapter::terminate_application(
    const std::string& /*app_identifier*/,
    bool /*force*/
) {
    return contracts::Result<void>::success();
}

contracts::Result<void> LinuxSystemAdapter::focus_application(const std::string& /*app_identifier*/) {
    return contracts::Result<void>::success();
}

contracts::Result<std::vector<contracts::ApplicationMetadata>> LinuxSystemAdapter::list_installed_applications() {
    std::vector<contracts::ApplicationMetadata> list;
    list.push_back({"org.mozilla.firefox", "Firefox", {"firefox", "browser"}, "/usr/bin/firefox", "/usr/bin/firefox", "firefox", "", "linux", false, {}, {}, "1.0", {}});
    return contracts::Result<std::vector<contracts::ApplicationMetadata>>::success(list);
}

contracts::Result<std::vector<contracts::ApplicationMetadata>> LinuxSystemAdapter::list_running_applications() {
    return contracts::Result<std::vector<contracts::ApplicationMetadata>>::success({});
}

contracts::Result<std::vector<contracts::ProcessMetadata>> LinuxSystemAdapter::list_processes() {
    std::vector<contracts::ProcessMetadata> procs;
    procs.push_back({1, "systemd", "/sbin/init", 0, 0.0, 15 * 1024 * 1024, 0, contracts::ProcessStatus::Running, "root", {"/sbin/init"}});
    return contracts::Result<std::vector<contracts::ProcessMetadata>>::success(procs);
}

contracts::Result<contracts::ProcessMetadata> LinuxSystemAdapter::inspect_process(uint32_t pid) {
    contracts::ProcessMetadata meta;
    meta.pid = pid;
    meta.name = "process_" + std::to_string(pid);
    meta.status = contracts::ProcessStatus::Running;
    return contracts::Result<contracts::ProcessMetadata>::success(meta);
}

contracts::Result<uint32_t> LinuxSystemAdapter::start_process(
    const std::string& /*command*/,
    const std::string& /*working_dir*/,
    const std::unordered_map<std::string, std::string>& /*env*/
) {
    return contracts::Result<uint32_t>::success(1024);
}

contracts::Result<void> LinuxSystemAdapter::stop_process(uint32_t /*pid*/, bool /*force*/) {
    return contracts::Result<void>::success();
}

contracts::Result<std::string> LinuxSystemAdapter::read_file(const std::string& /*path*/) {
    return contracts::Result<std::string>::success("linux file content");
}

contracts::Result<void> LinuxSystemAdapter::write_file(const std::string& /*path*/, const std::string& /*content*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> LinuxSystemAdapter::create_directory(const std::string& /*path*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> LinuxSystemAdapter::move_file(const std::string& /*source*/, const std::string& /*dest*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> LinuxSystemAdapter::copy_file(const std::string& /*source*/, const std::string& /*dest*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> LinuxSystemAdapter::delete_file(const std::string& /*path*/, bool /*use_trash*/) {
    return contracts::Result<void>::success();
}

contracts::Result<contracts::FileMetadata> LinuxSystemAdapter::get_file_metadata(const std::string& path) {
    contracts::FileMetadata meta;
    meta.path = path;
    meta.name = "file.txt";
    meta.size_bytes = 1024;
    meta.type = contracts::FileType::Regular;
    return contracts::Result<contracts::FileMetadata>::success(meta);
}

contracts::Result<std::vector<contracts::FileMetadata>> LinuxSystemAdapter::search_files(
    const std::string& /*root_path*/,
    const std::string& /*pattern*/
) {
    return contracts::Result<std::vector<contracts::FileMetadata>>::success({});
}

contracts::Result<contracts::TerminalExecutionResult> LinuxSystemAdapter::execute_command(
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
    res.stdout_content = "Linux Command Executed";
    res.duration_ms = 10;
    res.process_id = 888;
    return contracts::Result<contracts::TerminalExecutionResult>::success(res);
}

contracts::Result<std::vector<contracts::WindowMetadata>> LinuxSystemAdapter::list_windows() {
    return contracts::Result<std::vector<contracts::WindowMetadata>>::success({});
}

contracts::Result<void> LinuxSystemAdapter::focus_window(uint32_t /*window_id*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> LinuxSystemAdapter::set_window_state(uint32_t /*window_id*/, contracts::WindowState /*state*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> LinuxSystemAdapter::resize_window(uint32_t /*window_id*/, const contracts::WindowRect& /*rect*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> LinuxSystemAdapter::close_window(uint32_t /*window_id*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> LinuxSystemAdapter::inject_key_press(
    const std::string& /*key*/,
    const contracts::InputTargetContext& /*target*/
) {
    return contracts::Result<void>::success();
}

contracts::Result<void> LinuxSystemAdapter::inject_key_sequence(
    const std::string& /*text*/,
    const contracts::InputTargetContext& /*target*/
) {
    return contracts::Result<void>::success();
}

contracts::Result<void> LinuxSystemAdapter::inject_mouse_click(
    contracts::MouseButton /*button*/,
    int32_t /*x*/,
    int32_t /*y*/,
    const contracts::InputTargetContext& /*target*/
) {
    return contracts::Result<void>::success();
}

contracts::Result<void> LinuxSystemAdapter::inject_mouse_move(
    int32_t /*x*/,
    int32_t /*y*/,
    const contracts::InputTargetContext& /*target*/
) {
    return contracts::Result<void>::success();
}

contracts::Result<void> LinuxSystemAdapter::inject_mouse_scroll(
    int32_t /*delta_y*/,
    const contracts::InputTargetContext& /*target*/
) {
    return contracts::Result<void>::success();
}

contracts::Result<contracts::ClipboardPayload> LinuxSystemAdapter::read_clipboard() {
    contracts::ClipboardPayload payload;
    payload.type = contracts::ClipboardContentType::Text;
    payload.text_content = "Linux clipboard text";
    return contracts::Result<contracts::ClipboardPayload>::success(payload);
}

contracts::Result<void> LinuxSystemAdapter::write_clipboard(const contracts::ClipboardPayload& /*payload*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> LinuxSystemAdapter::clear_clipboard() {
    return contracts::Result<void>::success();
}

contracts::Result<contracts::ScreenCaptureMetadata> LinuxSystemAdapter::capture_screen(
    uint32_t monitor_id,
    uint32_t window_id
) {
    contracts::ScreenCaptureMetadata meta;
    meta.capture_id = "linux_cap_001";
    meta.monitor_id = monitor_id;
    meta.window_id = window_id;
    meta.width = 1920;
    meta.height = 1080;
    meta.format = "png";
    meta.source = "x11_screen_capture";
    return contracts::Result<contracts::ScreenCaptureMetadata>::success(meta);
}

contracts::Result<std::vector<contracts::DisplayMetadata>> LinuxSystemAdapter::list_displays() {
    std::vector<contracts::DisplayMetadata> list;
    list.push_back({0, "eDP-1", 1920, 1080, 60, 100, true});
    return contracts::Result<std::vector<contracts::DisplayMetadata>>::success(list);
}

contracts::Result<uint32_t> LinuxSystemAdapter::get_brightness(uint32_t /*display_id*/) {
    return contracts::Result<uint32_t>::success(100);
}

contracts::Result<void> LinuxSystemAdapter::set_brightness(uint32_t /*display_id*/, uint32_t /*percent*/) {
    return contracts::Result<void>::success();
}

contracts::Result<contracts::MediaStatus> LinuxSystemAdapter::get_media_status() {
    contracts::MediaStatus s;
    s.state = contracts::MediaPlaybackState::Stopped;
    s.volume_percent = 50;
    return contracts::Result<contracts::MediaStatus>::success(s);
}

contracts::Result<void> LinuxSystemAdapter::media_play() { return contracts::Result<void>::success(); }
contracts::Result<void> LinuxSystemAdapter::media_pause() { return contracts::Result<void>::success(); }
contracts::Result<void> LinuxSystemAdapter::media_stop() { return contracts::Result<void>::success(); }
contracts::Result<void> LinuxSystemAdapter::media_next() { return contracts::Result<void>::success(); }
contracts::Result<void> LinuxSystemAdapter::media_previous() { return contracts::Result<void>::success(); }
contracts::Result<void> LinuxSystemAdapter::set_volume(uint32_t /*volume_percent*/) { return contracts::Result<void>::success(); }

contracts::Result<contracts::SystemStateSnapshot> LinuxSystemAdapter::get_system_state() {
    contracts::SystemStateSnapshot s;
    s.os_name = "Linux";
    s.os_version = "6.5.0";
    s.logged_in_user = "user";
    s.is_online = true;
    return contracts::Result<contracts::SystemStateSnapshot>::success(s);
}

contracts::Result<void> LinuxSystemAdapter::execute_power_action(contracts::PowerAction /*action*/) {
    return contracts::Result<void>::success();
}

contracts::Result<void> LinuxSystemAdapter::send_notification(const contracts::NotificationPayload& /*notification*/) {
    return contracts::Result<void>::success();
}

} // namespace vani::adapters::system
