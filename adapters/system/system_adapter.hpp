#pragma once

#include "../../contracts/capabilities/capability_descriptor.hpp"
#include "../../contracts/tools/risk_level.hpp"
#include "../../contracts/system/system_contracts.hpp"
#include "../../contracts/common/result.hpp"
#include "../../contracts/common/cancellation_token.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace vani::adapters::system {

class SystemAdapter {
public:
    virtual ~SystemAdapter() = default;

    // Platform Identity
    [[nodiscard]] virtual contracts::PlatformId platform_id() const noexcept = 0;
    [[nodiscard]] virtual std::string platform_name() const noexcept = 0;

    // 1. Applications
    virtual contracts::Result<contracts::ApplicationMetadata> launch_application(
        const std::string& app_identifier,
        const std::vector<std::string>& args = {}
    ) = 0;
    virtual contracts::Result<void> terminate_application(
        const std::string& app_identifier,
        bool force = false
    ) = 0;
    virtual contracts::Result<void> focus_application(const std::string& app_identifier) = 0;
    virtual contracts::Result<std::vector<contracts::ApplicationMetadata>> list_installed_applications() = 0;
    virtual contracts::Result<std::vector<contracts::ApplicationMetadata>> list_running_applications() = 0;

    // 2. Processes
    virtual contracts::Result<std::vector<contracts::ProcessMetadata>> list_processes() = 0;
    virtual contracts::Result<contracts::ProcessMetadata> inspect_process(uint32_t pid) = 0;
    virtual contracts::Result<uint32_t> start_process(
        const std::string& command,
        const std::string& working_dir,
        const std::unordered_map<std::string, std::string>& env
    ) = 0;
    virtual contracts::Result<void> stop_process(uint32_t pid, bool force = false) = 0;

    // 3. Filesystem
    virtual contracts::Result<std::string> read_file(const std::string& path) = 0;
    virtual contracts::Result<void> write_file(const std::string& path, const std::string& content) = 0;
    virtual contracts::Result<void> create_directory(const std::string& path) = 0;
    virtual contracts::Result<void> move_file(const std::string& source, const std::string& dest) = 0;
    virtual contracts::Result<void> copy_file(const std::string& source, const std::string& dest) = 0;
    virtual contracts::Result<void> delete_file(const std::string& path, bool use_trash = true) = 0;
    virtual contracts::Result<contracts::FileMetadata> get_file_metadata(const std::string& path) = 0;
    virtual contracts::Result<std::vector<contracts::FileMetadata>> search_files(
        const std::string& root_path,
        const std::string& pattern
    ) = 0;

    // 4. Terminal
    virtual contracts::Result<contracts::TerminalExecutionResult> execute_command(
        const contracts::TerminalExecutionRequest& request,
        const contracts::CancellationToken& cancel_token = contracts::CancellationToken::none()
    ) = 0;

    // 5. Windows
    virtual contracts::Result<std::vector<contracts::WindowMetadata>> list_windows() = 0;
    virtual contracts::Result<void> focus_window(uint32_t window_id) = 0;
    virtual contracts::Result<void> set_window_state(uint32_t window_id, contracts::WindowState state) = 0;
    virtual contracts::Result<void> resize_window(uint32_t window_id, const contracts::WindowRect& rect) = 0;
    virtual contracts::Result<void> close_window(uint32_t window_id) = 0;

    // 6. Input
    virtual contracts::Result<void> inject_key_press(
        const std::string& key,
        const contracts::InputTargetContext& target
    ) = 0;
    virtual contracts::Result<void> inject_key_sequence(
        const std::string& text,
        const contracts::InputTargetContext& target
    ) = 0;
    virtual contracts::Result<void> inject_mouse_click(
        contracts::MouseButton button,
        int32_t x,
        int32_t y,
        const contracts::InputTargetContext& target
    ) = 0;
    virtual contracts::Result<void> inject_mouse_move(
        int32_t x,
        int32_t y,
        const contracts::InputTargetContext& target
    ) = 0;
    virtual contracts::Result<void> inject_mouse_scroll(
        int32_t delta_y,
        const contracts::InputTargetContext& target
    ) = 0;

    // 7. Clipboard
    virtual contracts::Result<contracts::ClipboardPayload> read_clipboard() = 0;
    virtual contracts::Result<void> write_clipboard(const contracts::ClipboardPayload& payload) = 0;
    virtual contracts::Result<void> clear_clipboard() = 0;

    // 8. Screen Capture
    virtual contracts::Result<contracts::ScreenCaptureMetadata> capture_screen(
        uint32_t monitor_id = 0,
        uint32_t window_id = 0
    ) = 0;

    // 9. Display
    virtual contracts::Result<std::vector<contracts::DisplayMetadata>> list_displays() = 0;
    virtual contracts::Result<uint32_t> get_brightness(uint32_t display_id) = 0;
    virtual contracts::Result<void> set_brightness(uint32_t display_id, uint32_t percent) = 0;

    // 10. Media
    virtual contracts::Result<contracts::MediaStatus> get_media_status() = 0;
    virtual contracts::Result<void> media_play() = 0;
    virtual contracts::Result<void> media_pause() = 0;
    virtual contracts::Result<void> media_stop() = 0;
    virtual contracts::Result<void> media_next() = 0;
    virtual contracts::Result<void> media_previous() = 0;
    virtual contracts::Result<void> set_volume(uint32_t volume_percent) = 0;

    // 11. System State
    virtual contracts::Result<contracts::SystemStateSnapshot> get_system_state() = 0;

    // 12. Power
    virtual contracts::Result<void> execute_power_action(contracts::PowerAction action) = 0;

    // 13. Notifications
    virtual contracts::Result<void> send_notification(const contracts::NotificationPayload& notification) = 0;
};

using SystemAdapterPtr = std::shared_ptr<SystemAdapter>;

} // namespace vani::adapters::system
