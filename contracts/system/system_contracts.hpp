#pragma once

#include "../common/result.hpp"
#include "../common/version.hpp"
#include "../tools/risk_level.hpp"
#include "../capabilities/capability_descriptor.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <optional>

namespace vani::contracts {

// ==========================================
// 1. Applications
// ==========================================
struct ApplicationMetadata {
    std::string application_id;
    std::string name;
    std::vector<std::string> aliases;
    std::string executable;
    std::string path;
    std::string launch_command;
    std::string icon_path;
    std::string platform;
    bool is_running{false};
    std::vector<uint32_t> window_ids;
    std::vector<uint32_t> process_ids;
    std::string version;
    std::vector<std::string> supported_capabilities;
};

// ==========================================
// 2. Processes
// ==========================================
enum class ProcessStatus : uint8_t {
    Running,
    Sleeping,
    Stopped,
    Zombie,
    Terminated,
    Unknown
};

struct ProcessMetadata {
    uint32_t pid{0};
    std::string name;
    std::string path;
    uint32_t parent_pid{0};
    double cpu_percent{0.0};
    uint64_t memory_bytes{0};
    uint64_t start_time_ms{0};
    ProcessStatus status{ProcessStatus::Running};
    std::string user;
    std::vector<std::string> command_line;
};

// ==========================================
// 3. Filesystem
// ==========================================
enum class FileType : uint8_t {
    Regular,
    Directory,
    Symlink,
    Other
};

struct FileMetadata {
    std::string path;
    std::string name;
    uint64_t size_bytes{0};
    FileType type{FileType::Regular};
    uint64_t created_at_ms{0};
    uint64_t modified_at_ms{0};
    uint32_t permissions_mode{0};
    bool is_readonly{false};
    bool is_hidden{false};
    std::string sha256_checksum;
};

enum class FileWatchEvent : uint8_t {
    Created,
    Modified,
    Deleted,
    Renamed
};

struct FileWatchNotification {
    std::string path;
    FileWatchEvent event{FileWatchEvent::Modified};
    uint64_t timestamp_ms{0};
};

// ==========================================
// 4. Terminal
// ==========================================
enum class TerminalExecutionMode : uint8_t {
    Normal,
    Restricted,
    Sandboxed
};

enum class CommandRiskCategory : uint8_t {
    Safe,       // ls, pwd, git status
    Low,        // git diff, echo, cat
    Medium,     // npm test, build, compile
    High,       // rm, git reset, chmod, package install
    Critical    // format, shutdown, system modifications
};

struct TerminalExecutionRequest {
    std::string command;
    std::string working_directory;
    std::unordered_map<std::string, std::string> environment;
    uint32_t timeout_ms{30000};
    uint64_t max_output_bytes{1024 * 1024};
    TerminalExecutionMode execution_mode{TerminalExecutionMode::Normal};
    bool capture_output{true};
};

struct TerminalExecutionResult {
    std::string command;
    std::string cwd;
    int32_t exit_code{0};
    std::string stdout_content;
    std::string stderr_content;
    uint32_t duration_ms{0};
    uint32_t process_id{0};
    bool timed_out{false};
    bool cancelled{false};
    bool output_truncated{false};
};

// ==========================================
// 5. Browser
// ==========================================
struct BrowserSessionMetadata {
    std::string session_id;
    std::string browser_name;
    std::string profile_name;
    std::vector<std::string> open_tabs;
    std::string active_url;
    std::string task_id;
    bool is_headless{false};
};

// ==========================================
// 6. Windows
// ==========================================
enum class WindowState : uint8_t {
    Normal,
    Minimized,
    Maximized,
    Hidden
};

struct WindowRect {
    int32_t x{0};
    int32_t y{0};
    uint32_t width{800};
    uint32_t height{600};
};

struct WindowMetadata {
    uint32_t window_id{0};
    std::string application_id;
    std::string title;
    WindowRect rect{};
    uint32_t monitor_id{0};
    WindowState state{WindowState::Normal};
    bool is_focused{false};
};

// ==========================================
// 7. Input
// ==========================================
enum class MouseButton : uint8_t {
    Left,
    Right,
    Middle
};

struct InputTargetContext {
    std::string application_id;
    uint32_t window_id{0};
    std::string browser_tab_id;
    uint32_t monitor_id{0};
};

// ==========================================
// 8. Clipboard
// ==========================================
enum class ClipboardContentType : uint8_t {
    Text,
    Html,
    Image,
    Files,
    Empty
};

struct ClipboardPayload {
    ClipboardContentType type{ClipboardContentType::Text};
    std::string text_content;
    std::vector<std::string> file_paths;
    bool contains_sensitive_data{false};
};

// ==========================================
// 9. Screen Capture
// ==========================================
struct ScreenCaptureMetadata {
    std::string capture_id;
    uint64_t timestamp_ms{0};
    uint32_t monitor_id{0};
    uint32_t window_id{0};
    uint32_t width{0};
    uint32_t height{0};
    std::string format{"png"}; // png, raw_rgb
    std::string source{"desktop"};
    std::vector<uint8_t> image_data;
};

// ==========================================
// 10. Display
// ==========================================
struct DisplayMetadata {
    uint32_t display_id{0};
    std::string name;
    uint32_t width{1920};
    uint32_t height{1080};
    uint32_t refresh_rate_hz{60};
    uint32_t brightness_percent{100};
    bool is_primary{true};
};

// ==========================================
// 11. Media
// ==========================================
enum class MediaPlaybackState : uint8_t {
    Playing,
    Paused,
    Stopped,
    Unknown
};

struct MediaStatus {
    MediaPlaybackState state{MediaPlaybackState::Stopped};
    std::string current_track;
    std::string artist;
    uint32_t volume_percent{50};
    bool is_muted{false};
};

// ==========================================
// 12. System State
// ==========================================
struct SystemStateSnapshot {
    double cpu_usage_percent{0.0};
    uint64_t ram_used_bytes{0};
    uint64_t ram_total_bytes{0};
    double gpu_usage_percent{0.0};
    uint64_t vram_used_bytes{0};
    uint64_t vram_total_bytes{0};
    uint64_t disk_used_bytes{0};
    uint64_t disk_total_bytes{0};
    uint32_t battery_percent{100};
    bool is_battery_charging{true};
    bool is_online{true};
    std::string active_window_title;
    uint64_t uptime_seconds{0};
    std::string os_name;
    std::string os_version;
    std::string logged_in_user;
    uint32_t display_count{1};
};

// ==========================================
// 13. Power
// ==========================================
enum class PowerAction : uint8_t {
    Lock,
    Sleep,
    Restart,
    Shutdown
};

// ==========================================
// 14. Notifications
// ==========================================
enum class NotificationPriority : uint8_t {
    Low,
    Normal,
    High,
    Critical
};

struct NotificationPayload {
    std::string id;
    std::string title;
    std::string message;
    NotificationPriority priority{NotificationPriority::Normal};
    std::string category{"system"};
    std::string task_id;
    std::string icon_path;
    uint32_t duration_ms{5000};
};

// ==========================================
// 15. Action Journal Record
// ==========================================
struct SystemActionJournalEntry {
    std::string entry_id;
    uint64_t timestamp_ms{0};
    std::string task_id;
    std::string capability_id;
    std::string actor_id;
    std::string arguments_summary; // Sanitized: zero secrets
    RiskLevel risk_level{RiskLevel::Low};
    std::string policy_decision;
    bool execution_success{false};
    std::string verification_result;
    uint32_t duration_ms{0};
    bool is_rollback{false};
};

} // namespace vani::contracts
