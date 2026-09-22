#pragma once

#include "../system_adapter.hpp"

namespace vani::adapters::system {

class LinuxSystemAdapter : public SystemAdapter {
public:
    LinuxSystemAdapter();
    ~LinuxSystemAdapter() override = default;

    [[nodiscard]] contracts::PlatformId platform_id() const noexcept override;
    [[nodiscard]] std::string platform_name() const noexcept override;

    contracts::Result<contracts::ApplicationMetadata> launch_application(
        const std::string& app_identifier,
        const std::vector<std::string>& args = {}
    ) override;
    contracts::Result<void> terminate_application(
        const std::string& app_identifier,
        bool force = false
    ) override;
    contracts::Result<void> focus_application(const std::string& app_identifier) override;
    contracts::Result<std::vector<contracts::ApplicationMetadata>> list_installed_applications() override;
    contracts::Result<std::vector<contracts::ApplicationMetadata>> list_running_applications() override;

    contracts::Result<std::vector<contracts::ProcessMetadata>> list_processes() override;
    contracts::Result<contracts::ProcessMetadata> inspect_process(uint32_t pid) override;
    contracts::Result<uint32_t> start_process(
        const std::string& command,
        const std::string& working_dir,
        const std::unordered_map<std::string, std::string>& env
    ) override;
    contracts::Result<void> stop_process(uint32_t pid, bool force = false) override;

    contracts::Result<std::string> read_file(const std::string& path) override;
    contracts::Result<void> write_file(const std::string& path, const std::string& content) override;
    contracts::Result<void> create_directory(const std::string& path) override;
    contracts::Result<void> move_file(const std::string& source, const std::string& dest) override;
    contracts::Result<void> copy_file(const std::string& source, const std::string& dest) override;
    contracts::Result<void> delete_file(const std::string& path, bool use_trash = true) override;
    contracts::Result<contracts::FileMetadata> get_file_metadata(const std::string& path) override;
    contracts::Result<std::vector<contracts::FileMetadata>> search_files(
        const std::string& root_path,
        const std::string& pattern
    ) override;

    contracts::Result<contracts::TerminalExecutionResult> execute_command(
        const contracts::TerminalExecutionRequest& request,
        const contracts::CancellationToken& cancel_token = contracts::CancellationToken::none()
    ) override;

    contracts::Result<std::vector<contracts::WindowMetadata>> list_windows() override;
    contracts::Result<void> focus_window(uint32_t window_id) override;
    contracts::Result<void> set_window_state(uint32_t window_id, contracts::WindowState state) override;
    contracts::Result<void> resize_window(uint32_t window_id, const contracts::WindowRect& rect) override;
    contracts::Result<void> close_window(uint32_t window_id) override;

    contracts::Result<void> inject_key_press(
        const std::string& key,
        const contracts::InputTargetContext& target
    ) override;
    contracts::Result<void> inject_key_sequence(
        const std::string& text,
        const contracts::InputTargetContext& target
    ) override;
    contracts::Result<void> inject_mouse_click(
        contracts::MouseButton button,
        int32_t x,
        int32_t y,
        const contracts::InputTargetContext& target
    ) override;
    contracts::Result<void> inject_mouse_move(
        int32_t x,
        int32_t y,
        const contracts::InputTargetContext& target
    ) override;
    contracts::Result<void> inject_mouse_scroll(
        int32_t delta_y,
        const contracts::InputTargetContext& target
    ) override;

    contracts::Result<contracts::ClipboardPayload> read_clipboard() override;
    contracts::Result<void> write_clipboard(const contracts::ClipboardPayload& payload) override;
    contracts::Result<void> clear_clipboard() override;

    contracts::Result<contracts::ScreenCaptureMetadata> capture_screen(
        uint32_t monitor_id = 0,
        uint32_t window_id = 0
    ) override;

    contracts::Result<std::vector<contracts::DisplayMetadata>> list_displays() override;
    contracts::Result<uint32_t> get_brightness(uint32_t display_id) override;
    contracts::Result<void> set_brightness(uint32_t display_id, uint32_t percent) override;

    contracts::Result<contracts::MediaStatus> get_media_status() override;
    contracts::Result<void> media_play() override;
    contracts::Result<void> media_pause() override;
    contracts::Result<void> media_stop() override;
    contracts::Result<void> media_next() override;
    contracts::Result<void> media_previous() override;
    contracts::Result<void> set_volume(uint32_t volume_percent) override;

    contracts::Result<contracts::SystemStateSnapshot> get_system_state() override;

    contracts::Result<void> execute_power_action(contracts::PowerAction action) override;

    contracts::Result<void> send_notification(const contracts::NotificationPayload& notification) override;
};

} // namespace vani::adapters::system
