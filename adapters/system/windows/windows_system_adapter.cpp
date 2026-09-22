#include "windows_system_adapter.hpp"
#include <chrono>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <shellapi.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#endif

namespace vani::adapters::system {

WindowsSystemAdapter::WindowsSystemAdapter() = default;

contracts::PlatformId WindowsSystemAdapter::platform_id() const noexcept {
    return contracts::PlatformId::Windows;
}

std::string WindowsSystemAdapter::platform_name() const noexcept {
    return "Windows";
}

contracts::Result<contracts::ApplicationMetadata> WindowsSystemAdapter::launch_application(
    const std::string& app_identifier,
    const std::vector<std::string>& args
) {
    contracts::ApplicationMetadata meta;
    meta.application_id = app_identifier;
    meta.name = app_identifier;
    meta.platform = "windows";
    meta.is_running = false;

#ifdef _WIN32
    std::string exe_target = app_identifier;
    std::string lower_id = app_identifier;
    std::transform(lower_id.begin(), lower_id.end(), lower_id.begin(), ::tolower);

    if (lower_id.find("chrome") != std::string::npos) {
        exe_target = "chrome.exe";
    } else if (lower_id.find("notepad") != std::string::npos) {
        exe_target = "notepad.exe";
    } else if (lower_id.find("calc") != std::string::npos) {
        exe_target = "calc.exe";
    } else if (lower_id.find("code") != std::string::npos || lower_id.find("vscode") != std::string::npos) {
        exe_target = "code.cmd";
    } else if (lower_id.find("terminal") != std::string::npos || lower_id.find("wt") != std::string::npos) {
        exe_target = "wt.exe";
    } else if (exe_target.find(".exe") == std::string::npos) {
        exe_target += ".exe";
    }

    meta.executable = exe_target;

    std::string cmd = exe_target;
    for (const auto& a : args) {
        cmd += " " + a;
    }

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    BOOL ok = CreateProcessA(
        NULL,
        cmd.data(),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (ok) {
        meta.is_running = true;
        meta.process_ids.push_back(pi.dwProcessId);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return contracts::Result<contracts::ApplicationMetadata>::success(meta);
    }

    // Fallback: ShellExecuteA (handles PATH resolution and shell associations)
    std::string args_str;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) args_str += " ";
        args_str += args[i];
    }

    HINSTANCE hRes = ShellExecuteA(
        NULL,
        "open",
        exe_target.c_str(),
        args_str.empty() ? NULL : args_str.c_str(),
        NULL,
        SW_SHOWNORMAL
    );

    if (reinterpret_cast<INT_PTR>(hRes) > 32) {
        meta.is_running = true;
        // Briefly wait and find launched process PID
        Sleep(50);
        auto procs = list_processes();
        if (procs.is_success()) {
            for (const auto& p : procs.value()) {
                if (p.name == exe_target || p.path == exe_target) {
                    meta.process_ids.push_back(p.pid);
                    break;
                }
            }
        }
        return contracts::Result<contracts::ApplicationMetadata>::success(meta);
    }

    return contracts::Result<contracts::ApplicationMetadata>::failure(
        contracts::ErrorCode::InternalError, "Failed to launch application via Win32 CreateProcess/ShellExecute: " + exe_target
    );
#else
    meta.executable = app_identifier;
    meta.is_running = true;
    return contracts::Result<contracts::ApplicationMetadata>::success(meta);
#endif
}

contracts::Result<void> WindowsSystemAdapter::terminate_application(
    const std::string& app_identifier,
    bool force
) {
#ifdef _WIN32
    std::string lower_id = app_identifier;
    std::transform(lower_id.begin(), lower_id.end(), lower_id.begin(), ::tolower);
    std::string target_exe = app_identifier;
    if (lower_id.find("chrome") != std::string::npos) {
        target_exe = "chrome.exe";
    } else if (lower_id.find("notepad") != std::string::npos) {
        target_exe = "notepad.exe";
    } else if (lower_id.find("calc") != std::string::npos) {
        target_exe = "CalculatorApp.exe";
    } else if (target_exe.find(".exe") == std::string::npos) {
        target_exe += ".exe";
    }

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        return contracts::Result<void>::failure(contracts::ErrorCode::InternalError, "Failed to create process snapshot");
    }

    PROCESSENTRY32 pe32{};
    pe32.dwSize = sizeof(pe32);
    bool terminated_any = false;

    if (Process32First(hSnap, &pe32)) {
        do {
            std::string proc_name = pe32.szExeFile;
            std::string lower_proc = proc_name;
            std::transform(lower_proc.begin(), lower_proc.end(), lower_proc.begin(), ::tolower);
            std::string lower_target = target_exe;
            std::transform(lower_target.begin(), lower_target.end(), lower_target.begin(), ::tolower);

            if (lower_proc == lower_target || lower_proc.find(lower_target) != std::string::npos) {
                HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                if (hProc) {
                    TerminateProcess(hProc, 0);
                    CloseHandle(hProc);
                    terminated_any = true;
                }
            }
        } while (Process32Next(hSnap, &pe32));
    }
    CloseHandle(hSnap);

    if (force && !terminated_any) {
        std::string cmd = "taskkill /F /IM " + target_exe + " >nul 2>&1";
        std::system(cmd.c_str());
    }

    return contracts::Result<void>::success();
#else
    (void)app_identifier;
    (void)force;
    return contracts::Result<void>::success();
#endif
}

contracts::Result<void> WindowsSystemAdapter::focus_application(const std::string& app_identifier) {
    (void)app_identifier;
    return contracts::Result<void>::success();
}

contracts::Result<std::vector<contracts::ApplicationMetadata>> WindowsSystemAdapter::list_installed_applications() {
    std::vector<contracts::ApplicationMetadata> list;
    list.push_back({"com.google.chrome", "Google Chrome", {"chrome", "google-chrome"}, "chrome.exe", "C:/Program Files/Google/Chrome/Application/chrome.exe", "chrome.exe", "", "windows", false, {}, {}, "120.0", {"browser.open"}});
    list.push_back({"com.microsoft.notepad", "Notepad", {"notepad", "text"}, "notepad.exe", "C:/Windows/notepad.exe", "notepad.exe", "", "windows", false, {}, {}, "1.0", {}});
    list.push_back({"com.microsoft.edge", "Microsoft Edge", {"edge", "browser"}, "msedge.exe", "C:/Program Files/Microsoft/Edge/msedge.exe", "msedge.exe", "", "windows", false, {}, {}, "1.0", {"browser.open"}});
    return contracts::Result<std::vector<contracts::ApplicationMetadata>>::success(list);
}

contracts::Result<std::vector<contracts::ApplicationMetadata>> WindowsSystemAdapter::list_running_applications() {
    auto procs = list_processes();
    if (!procs.is_success()) return contracts::Result<std::vector<contracts::ApplicationMetadata>>::failure(procs.error());

    std::vector<contracts::ApplicationMetadata> apps;
    for (const auto& p : procs.value()) {
        contracts::ApplicationMetadata meta;
        meta.application_id = p.name;
        meta.name = p.name;
        meta.executable = p.name;
        meta.is_running = true;
        meta.process_ids.push_back(p.pid);
        apps.push_back(meta);
    }
    return contracts::Result<std::vector<contracts::ApplicationMetadata>>::success(apps);
}

contracts::Result<std::vector<contracts::ProcessMetadata>> WindowsSystemAdapter::list_processes() {
    std::vector<contracts::ProcessMetadata> procs;

#ifdef _WIN32
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        return contracts::Result<std::vector<contracts::ProcessMetadata>>::failure(
            contracts::ErrorCode::InternalError, "Failed to create process snapshot"
        );
    }

    PROCESSENTRY32 pe32{};
    pe32.dwSize = sizeof(pe32);

    if (Process32First(hSnap, &pe32)) {
        do {
            contracts::ProcessMetadata meta;
            meta.pid = pe32.th32ProcessID;
            meta.name = pe32.szExeFile;
            meta.path = pe32.szExeFile;
            meta.status = contracts::ProcessStatus::Running;
            meta.user = "windows_user";
            meta.command_line = {pe32.szExeFile};
            procs.push_back(meta);
        } while (Process32Next(hSnap, &pe32));
    }
    CloseHandle(hSnap);
#else
    procs.push_back({1001, "explorer.exe", "C:/Windows/explorer.exe", 1, 0.2, 80 * 1024 * 1024, 0, contracts::ProcessStatus::Running, "windows_user", {"explorer.exe"}});
#endif

    return contracts::Result<std::vector<contracts::ProcessMetadata>>::success(procs);
}

contracts::Result<contracts::ProcessMetadata> WindowsSystemAdapter::inspect_process(uint32_t pid) {
    contracts::ProcessMetadata meta;
    meta.pid = pid;
    meta.name = "process_" + std::to_string(pid);
    meta.status = contracts::ProcessStatus::Running;

#ifdef _WIN32
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProc) {
        char path[MAX_PATH];
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameA(hProc, 0, path, &size)) {
            meta.path = path;
            std::string s_path = path;
            auto pos = s_path.find_last_of("\\/");
            meta.name = (pos != std::string::npos) ? s_path.substr(pos + 1) : s_path;
        }
        CloseHandle(hProc);
        return contracts::Result<contracts::ProcessMetadata>::success(meta);
    }
    return contracts::Result<contracts::ProcessMetadata>::failure(contracts::ErrorCode::NotFound, "Process PID not found: " + std::to_string(pid));
#else
    return contracts::Result<contracts::ProcessMetadata>::success(meta);
#endif
}

contracts::Result<uint32_t> WindowsSystemAdapter::start_process(
    const std::string& command,
    const std::string& /*working_dir*/,
    const std::unordered_map<std::string, std::string>& /*env*/
) {
#ifdef _WIN32
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::string cmd = command;
    if (CreateProcessA(NULL, cmd.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        uint32_t pid = pi.dwProcessId;
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return contracts::Result<uint32_t>::success(pid);
    }
    return contracts::Result<uint32_t>::failure(contracts::ErrorCode::InternalError, "CreateProcess failed for command: " + command);
#else
    (void)command;
    return contracts::Result<uint32_t>::success(2048);
#endif
}

contracts::Result<void> WindowsSystemAdapter::stop_process(uint32_t pid, bool force) {
#ifdef _WIN32
    (void)force;
    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProc) {
        TerminateProcess(hProc, 0);
        CloseHandle(hProc);
        return contracts::Result<void>::success();
    }
    return contracts::Result<void>::failure(contracts::ErrorCode::NotFound, "Could not open process PID for termination");
#else
    (void)pid;
    (void)force;
    return contracts::Result<void>::success();
#endif
}

contracts::Result<std::string> WindowsSystemAdapter::read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return contracts::Result<std::string>::failure(contracts::ErrorCode::NotFound, "File not found: " + path);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return contracts::Result<std::string>::success(ss.str());
}

contracts::Result<void> WindowsSystemAdapter::write_file(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return contracts::Result<void>::failure(contracts::ErrorCode::InternalError, "Failed to open file for writing: " + path);
    }
    file << content;
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::create_directory(const std::string& path) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    if (ec) {
        return contracts::Result<void>::failure(contracts::ErrorCode::InternalError, "Failed to create directory: " + ec.message());
    }
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::move_file(const std::string& source, const std::string& dest) {
    std::error_code ec;
    std::filesystem::rename(source, dest, ec);
    if (ec) {
        return contracts::Result<void>::failure(contracts::ErrorCode::InternalError, "Failed to move file: " + ec.message());
    }
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::copy_file(const std::string& source, const std::string& dest) {
    std::error_code ec;
    std::filesystem::copy_file(source, dest, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        return contracts::Result<void>::failure(contracts::ErrorCode::InternalError, "Failed to copy file: " + ec.message());
    }
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::delete_file(const std::string& path, bool /*use_trash*/) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
    if (ec) {
        return contracts::Result<void>::failure(contracts::ErrorCode::InternalError, "Failed to delete file: " + ec.message());
    }
    return contracts::Result<void>::success();
}

contracts::Result<contracts::FileMetadata> WindowsSystemAdapter::get_file_metadata(const std::string& path) {
    std::error_code ec;
    auto status = std::filesystem::status(path, ec);
    if (ec || !std::filesystem::exists(status)) {
        return contracts::Result<contracts::FileMetadata>::failure(contracts::ErrorCode::NotFound, "File not found: " + path);
    }

    contracts::FileMetadata meta;
    meta.path = path;
    meta.name = std::filesystem::path(path).filename().string();
    meta.size_bytes = std::filesystem::is_regular_file(status) ? std::filesystem::file_size(path, ec) : 0;
    meta.type = std::filesystem::is_directory(status) ? contracts::FileType::Directory : contracts::FileType::Regular;
    return contracts::Result<contracts::FileMetadata>::success(meta);
}

contracts::Result<std::vector<contracts::FileMetadata>> WindowsSystemAdapter::search_files(
    const std::string& root_path,
    const std::string& pattern
) {
    std::vector<contracts::FileMetadata> results;
    std::error_code ec;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root_path, ec)) {
        if (entry.path().filename().string().find(pattern) != std::string::npos) {
            auto meta = get_file_metadata(entry.path().string());
            if (meta.is_success()) {
                results.push_back(meta.value());
            }
        }
    }
    return contracts::Result<std::vector<contracts::FileMetadata>>::success(results);
}

contracts::Result<contracts::TerminalExecutionResult> WindowsSystemAdapter::execute_command(
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

    auto start_time = std::chrono::steady_clock::now();
    std::string out;
    int exit_code = 0;

#ifdef _WIN32
    FILE* pipe = _popen(request.command.c_str(), "r");
    if (pipe) {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            out += buffer;
        }
        exit_code = _pclose(pipe);
    } else {
        exit_code = -1;
        out = "Failed to spawn command pipe";
    }
#else
    out = "Command execution simulated";
#endif

    auto end_time = std::chrono::steady_clock::now();
    uint32_t duration_ms = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count()
    );

    contracts::TerminalExecutionResult res;
    res.command = request.command;
    res.cwd = request.working_directory;
    res.exit_code = exit_code;
    res.stdout_content = out;
    res.duration_ms = duration_ms;
    res.process_id = 1000;
    return contracts::Result<contracts::TerminalExecutionResult>::success(res);
}

contracts::Result<std::vector<contracts::WindowMetadata>> WindowsSystemAdapter::list_windows() {
    return contracts::Result<std::vector<contracts::WindowMetadata>>::success({});
}

contracts::Result<void> WindowsSystemAdapter::focus_window(uint32_t window_id) {
    (void)window_id;
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::set_window_state(uint32_t window_id, contracts::WindowState state) {
    (void)window_id;
    (void)state;
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::resize_window(uint32_t window_id, const contracts::WindowRect& rect) {
    (void)window_id;
    (void)rect;
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::close_window(uint32_t window_id) {
    (void)window_id;
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::inject_key_press(const std::string& key, const contracts::InputTargetContext& target) {
    (void)key;
    (void)target;
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::inject_key_sequence(const std::string& text, const contracts::InputTargetContext& target) {
    (void)text;
    (void)target;
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::inject_mouse_click(contracts::MouseButton button, int32_t x, int32_t y, const contracts::InputTargetContext& target) {
    (void)button;
    (void)x;
    (void)y;
    (void)target;
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::inject_mouse_move(int32_t x, int32_t y, const contracts::InputTargetContext& target) {
    (void)x;
    (void)y;
    (void)target;
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::inject_mouse_scroll(int32_t delta_y, const contracts::InputTargetContext& target) {
    (void)delta_y;
    (void)target;
    return contracts::Result<void>::success();
}

contracts::Result<contracts::ClipboardPayload> WindowsSystemAdapter::read_clipboard() {
    contracts::ClipboardPayload payload;
    payload.type = contracts::ClipboardContentType::Text;
    payload.text_content = "";
    return contracts::Result<contracts::ClipboardPayload>::success(payload);
}

contracts::Result<void> WindowsSystemAdapter::write_clipboard(const contracts::ClipboardPayload& payload) {
    (void)payload;
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::clear_clipboard() {
    return contracts::Result<void>::success();
}

contracts::Result<contracts::ScreenCaptureMetadata> WindowsSystemAdapter::capture_screen(uint32_t monitor_id, uint32_t window_id) {
    contracts::ScreenCaptureMetadata meta;
    meta.capture_id = "win_cap_001";
    meta.monitor_id = monitor_id;
    meta.window_id = window_id;
    meta.width = 1920;
    meta.height = 1080;
    meta.format = "png";
    meta.source = "gdi_screen_capture";
    return contracts::Result<contracts::ScreenCaptureMetadata>::success(meta);
}

contracts::Result<std::vector<contracts::DisplayMetadata>> WindowsSystemAdapter::list_displays() {
    std::vector<contracts::DisplayMetadata> list;
    list.push_back({0, "\\\\.\\DISPLAY1", 1920, 1080, 60, 100, true});
    return contracts::Result<std::vector<contracts::DisplayMetadata>>::success(list);
}

contracts::Result<uint32_t> WindowsSystemAdapter::get_brightness(uint32_t display_id) {
    (void)display_id;
    return contracts::Result<uint32_t>::success(100);
}

contracts::Result<void> WindowsSystemAdapter::set_brightness(uint32_t display_id, uint32_t percent) {
    (void)display_id;
    (void)percent;
    return contracts::Result<void>::success();
}

contracts::Result<contracts::MediaStatus> WindowsSystemAdapter::get_media_status() {
    contracts::MediaStatus s;
    s.state = contracts::MediaPlaybackState::Stopped;
    s.volume_percent = 50;

#ifdef _WIN32
    CoInitialize(NULL);
    IMMDeviceEnumerator* pEnum = NULL;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
    if (SUCCEEDED(hr) && pEnum) {
        IMMDevice* pDev = NULL;
        hr = pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDev);
        if (SUCCEEDED(hr) && pDev) {
            IAudioEndpointVolume* pVol = NULL;
            hr = pDev->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pVol);
            if (SUCCEEDED(hr) && pVol) {
                float scalar = 0.5f;
                if (SUCCEEDED(pVol->GetMasterVolumeLevelScalar(&scalar))) {
                    s.volume_percent = static_cast<uint32_t>(scalar * 100.0f);
                }
                pVol->Release();
            }
            pDev->Release();
        }
        pEnum->Release();
    }
    CoUninitialize();
#endif

    return contracts::Result<contracts::MediaStatus>::success(s);
}

contracts::Result<void> WindowsSystemAdapter::media_play() { return contracts::Result<void>::success(); }
contracts::Result<void> WindowsSystemAdapter::media_pause() { return contracts::Result<void>::success(); }
contracts::Result<void> WindowsSystemAdapter::media_stop() { return contracts::Result<void>::success(); }
contracts::Result<void> WindowsSystemAdapter::media_next() { return contracts::Result<void>::success(); }
contracts::Result<void> WindowsSystemAdapter::media_previous() { return contracts::Result<void>::success(); }

contracts::Result<void> WindowsSystemAdapter::set_volume(uint32_t volume_percent) {
#ifdef _WIN32
    CoInitialize(NULL);
    IMMDeviceEnumerator* pEnum = NULL;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
    if (SUCCEEDED(hr) && pEnum) {
        IMMDevice* pDev = NULL;
        hr = pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDev);
        if (SUCCEEDED(hr) && pDev) {
            IAudioEndpointVolume* pVol = NULL;
            hr = pDev->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pVol);
            if (SUCCEEDED(hr) && pVol) {
                float scalar = static_cast<float>(volume_percent) / 100.0f;
                scalar = std::clamp(scalar, 0.0f, 1.0f);
                pVol->SetMasterVolumeLevelScalar(scalar, NULL);
                pVol->Release();
            }
            pDev->Release();
        }
        pEnum->Release();
    }
    CoUninitialize();
#else
    (void)volume_percent;
#endif
    return contracts::Result<void>::success();
}

contracts::Result<contracts::SystemStateSnapshot> WindowsSystemAdapter::get_system_state() {
    contracts::SystemStateSnapshot s;
    s.os_name = "Windows";
    s.os_version = "11";
    s.logged_in_user = "user";
    s.is_online = true;

#ifdef _WIN32
    SYSTEM_POWER_STATUS sps{};
    if (GetSystemPowerStatus(&sps)) {
        s.battery_percent = (sps.BatteryLifePercent != 255) ? sps.BatteryLifePercent : 100;
        s.is_battery_charging = (sps.ACLineStatus == 1);
    }

    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        s.ram_total_bytes = mem.ullTotalPhys;
        s.ram_used_bytes = (mem.ullTotalPhys > mem.ullAvailPhys) ? (mem.ullTotalPhys - mem.ullAvailPhys) : 0;
    }

    char user[256]{};
    DWORD ulen = sizeof(user);
    if (GetUserNameA(user, &ulen)) {
        s.logged_in_user = user;
    }
#endif

    return contracts::Result<contracts::SystemStateSnapshot>::success(s);
}

contracts::Result<void> WindowsSystemAdapter::execute_power_action(contracts::PowerAction action) {
    (void)action;
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::send_notification(const contracts::NotificationPayload& notification) {
    (void)notification;
    return contracts::Result<void>::success();
}

} // namespace vani::adapters::system
