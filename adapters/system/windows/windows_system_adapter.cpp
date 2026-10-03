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
    std::error_code ec;
    std::filesystem::path fp(path);
    if (fp.has_parent_path()) {
        std::filesystem::create_directories(fp.parent_path(), ec);
    }
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

#ifdef _WIN32
struct EnumWindowsData {
    std::vector<contracts::WindowMetadata> windows;
    HWND focused_hwnd{nullptr};
};

static BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam) {
    if (!IsWindowVisible(hwnd)) return TRUE;
    
    int length = GetWindowTextLengthA(hwnd);
    if (length == 0) return TRUE;

    RECT rect;
    if (!GetWindowRect(hwnd, &rect)) return TRUE;
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    if (width <= 0 || height <= 0) return TRUE;

    std::string title(length + 1, '\0');
    GetWindowTextA(hwnd, &title[0], length + 1);
    title.resize(length);

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    auto* data = reinterpret_cast<EnumWindowsData*>(lParam);
    contracts::WindowMetadata meta;
    meta.window_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(hwnd));
    meta.application_id = std::to_string(pid);
    meta.title = title;
    meta.rect.x = rect.left;
    meta.rect.y = rect.top;
    meta.rect.width = static_cast<uint32_t>(width);
    meta.rect.height = static_cast<uint32_t>(height);
    meta.is_focused = (hwnd == data->focused_hwnd);
    if (IsIconic(hwnd)) {
        meta.state = contracts::WindowState::Minimized;
    } else if (IsZoomed(hwnd)) {
        meta.state = contracts::WindowState::Maximized;
    } else {
        meta.state = contracts::WindowState::Normal;
    }
    data->windows.push_back(meta);
    return TRUE;
}

struct DisplayEnumData {
    std::vector<contracts::DisplayMetadata> displays;
    uint32_t current_id{0};
};

static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC /*hdcMonitor*/, LPRECT /*lprcMonitor*/, LPARAM dwData) {
    auto* data = reinterpret_cast<DisplayEnumData*>(dwData);
    MONITORINFOEXA mi{};
    mi.cbSize = sizeof(MONITORINFOEXA);
    if (GetMonitorInfoA(hMonitor, &mi)) {
        contracts::DisplayMetadata disp;
        disp.display_id = data->current_id++;
        disp.name = mi.szDevice;
        disp.width = mi.rcMonitor.right - mi.rcMonitor.left;
        disp.height = mi.rcMonitor.bottom - mi.rcMonitor.top;
        disp.is_primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
        disp.brightness_percent = 100;
        disp.refresh_rate_hz = 60;
        data->displays.push_back(disp);
    }
    return TRUE;
}
#endif

contracts::Result<std::vector<contracts::WindowMetadata>> WindowsSystemAdapter::list_windows() {
#ifdef _WIN32
    EnumWindowsData data;
    data.focused_hwnd = GetForegroundWindow();
    EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&data));
    return contracts::Result<std::vector<contracts::WindowMetadata>>::success(data.windows);
#else
    return contracts::Result<std::vector<contracts::WindowMetadata>>::success({});
#endif
}

contracts::Result<void> WindowsSystemAdapter::focus_window(uint32_t window_id) {
#ifdef _WIN32
    HWND hwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(window_id));
    if (!IsWindow(hwnd)) {
        return contracts::Result<void>::failure(contracts::ErrorCode::NotFound, "Window not found: " + std::to_string(window_id));
    }
    if (IsIconic(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);
    }
    SetForegroundWindow(hwnd);
    BringWindowToTop(hwnd);
    return contracts::Result<void>::success();
#else
    (void)window_id;
    return contracts::Result<void>::success();
#endif
}

contracts::Result<void> WindowsSystemAdapter::set_window_state(uint32_t window_id, contracts::WindowState state) {
#ifdef _WIN32
    HWND hwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(window_id));
    if (!IsWindow(hwnd)) {
        return contracts::Result<void>::failure(contracts::ErrorCode::NotFound, "Window not found: " + std::to_string(window_id));
    }
    int cmd = SW_NORMAL;
    switch (state) {
        case contracts::WindowState::Minimized: cmd = SW_MINIMIZE; break;
        case contracts::WindowState::Maximized: cmd = SW_MAXIMIZE; break;
        case contracts::WindowState::Normal: cmd = SW_RESTORE; break;
        case contracts::WindowState::Hidden: cmd = SW_HIDE; break;
    }
    ShowWindow(hwnd, cmd);
    return contracts::Result<void>::success();
#else
    (void)window_id; (void)state;
    return contracts::Result<void>::success();
#endif
}

contracts::Result<void> WindowsSystemAdapter::resize_window(uint32_t window_id, const contracts::WindowRect& rect) {
#ifdef _WIN32
    HWND hwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(window_id));
    if (!IsWindow(hwnd)) {
        return contracts::Result<void>::failure(contracts::ErrorCode::NotFound, "Window not found: " + std::to_string(window_id));
    }
    MoveWindow(hwnd, rect.x, rect.y, rect.width, rect.height, TRUE);
    return contracts::Result<void>::success();
#else
    (void)window_id; (void)rect;
    return contracts::Result<void>::success();
#endif
}

contracts::Result<void> WindowsSystemAdapter::close_window(uint32_t window_id) {
#ifdef _WIN32
    HWND hwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(window_id));
    if (!IsWindow(hwnd)) {
        return contracts::Result<void>::failure(contracts::ErrorCode::NotFound, "Window not found: " + std::to_string(window_id));
    }
    PostMessage(hwnd, WM_CLOSE, 0, 0);
    return contracts::Result<void>::success();
#else
    (void)window_id;
    return contracts::Result<void>::success();
#endif
}

contracts::Result<void> WindowsSystemAdapter::inject_key_press(const std::string& key, const contracts::InputTargetContext& target) {
    (void)target;
#ifdef _WIN32
    WORD vk = 0;
    std::string k = key;
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);
    if (k == "enter" || k == "return") vk = VK_RETURN;
    else if (k == "space") vk = VK_SPACE;
    else if (k == "tab") vk = VK_TAB;
    else if (k == "escape" || k == "esc") vk = VK_ESCAPE;
    else if (k == "backspace") vk = VK_BACK;
    else if (k == "ctrl" || k == "control") vk = VK_CONTROL;
    else if (k == "alt") vk = VK_MENU;
    else if (k == "shift") vk = VK_SHIFT;
    else if (k == "win" || k == "super") vk = VK_LWIN;
    else if (k == "up") vk = VK_UP;
    else if (k == "down") vk = VK_DOWN;
    else if (k == "left") vk = VK_LEFT;
    else if (k == "right") vk = VK_RIGHT;
    else if (k.length() == 1) {
        SHORT s = VkKeyScanA(k[0]);
        if (s != -1) vk = LOBYTE(s);
    }
    if (vk == 0) vk = VK_RETURN;

    keybd_event(static_cast<BYTE>(vk), 0, 0, 0);
    keybd_event(static_cast<BYTE>(vk), 0, KEYEVENTF_KEYUP, 0);
    return contracts::Result<void>::success();
#else
    (void)key;
    return contracts::Result<void>::success();
#endif
}

contracts::Result<void> WindowsSystemAdapter::inject_key_sequence(const std::string& text, const contracts::InputTargetContext& target) {
    (void)target;
#ifdef _WIN32
    for (char c : text) {
        INPUT inputs[2]{};
        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wScan = static_cast<WORD>(static_cast<unsigned char>(c));
        inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;

        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wScan = static_cast<WORD>(static_cast<unsigned char>(c));
        inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

        SendInput(2, inputs, sizeof(INPUT));
    }
    return contracts::Result<void>::success();
#else
    (void)text;
    return contracts::Result<void>::success();
#endif
}

contracts::Result<void> WindowsSystemAdapter::inject_mouse_click(contracts::MouseButton button, int32_t x, int32_t y, const contracts::InputTargetContext& target) {
    (void)target;
#ifdef _WIN32
    if (x >= 0 && y >= 0) {
        SetCursorPos(x, y);
    }
    DWORD down_flag = MOUSEEVENTF_LEFTDOWN;
    DWORD up_flag = MOUSEEVENTF_LEFTUP;
    if (button == contracts::MouseButton::Right) {
        down_flag = MOUSEEVENTF_RIGHTDOWN;
        up_flag = MOUSEEVENTF_RIGHTUP;
    } else if (button == contracts::MouseButton::Middle) {
        down_flag = MOUSEEVENTF_MIDDLEDOWN;
        up_flag = MOUSEEVENTF_MIDDLEUP;
    }
    mouse_event(down_flag, 0, 0, 0, 0);
    mouse_event(up_flag, 0, 0, 0, 0);
    return contracts::Result<void>::success();
#else
    (void)button; (void)x; (void)y;
    return contracts::Result<void>::success();
#endif
}

contracts::Result<void> WindowsSystemAdapter::inject_mouse_move(int32_t x, int32_t y, const contracts::InputTargetContext& target) {
    (void)target;
#ifdef _WIN32
    SetCursorPos(x, y);
    return contracts::Result<void>::success();
#else
    (void)x; (void)y;
    return contracts::Result<void>::success();
#endif
}

contracts::Result<void> WindowsSystemAdapter::inject_mouse_scroll(int32_t delta_y, const contracts::InputTargetContext& target) {
    (void)target;
#ifdef _WIN32
    mouse_event(MOUSEEVENTF_WHEEL, 0, 0, static_cast<DWORD>(delta_y * 120), 0);
    return contracts::Result<void>::success();
#else
    (void)delta_y;
    return contracts::Result<void>::success();
#endif
}

contracts::Result<contracts::ClipboardPayload> WindowsSystemAdapter::read_clipboard() {
    contracts::ClipboardPayload payload;
    payload.type = contracts::ClipboardContentType::Text;
    payload.text_content = "";
#ifdef _WIN32
    if (OpenClipboard(NULL)) {
        HANDLE hData = GetClipboardData(CF_TEXT);
        if (hData) {
            char* pszText = static_cast<char*>(GlobalLock(hData));
            if (pszText) {
                payload.text_content = pszText;
                GlobalUnlock(hData);
            }
        }
        CloseClipboard();
    }
#endif
    return contracts::Result<contracts::ClipboardPayload>::success(payload);
}

contracts::Result<void> WindowsSystemAdapter::write_clipboard(const contracts::ClipboardPayload& payload) {
#ifdef _WIN32
    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, payload.text_content.size() + 1);
        if (hGlob) {
            char* dest = static_cast<char*>(GlobalLock(hGlob));
            if (dest) {
                memcpy(dest, payload.text_content.c_str(), payload.text_content.size() + 1);
                GlobalUnlock(hGlob);
                SetClipboardData(CF_TEXT, hGlob);
            }
        }
        CloseClipboard();
        return contracts::Result<void>::success();
    }
    return contracts::Result<void>::failure(contracts::ErrorCode::InternalError, "Failed to open clipboard");
#else
    (void)payload;
    return contracts::Result<void>::success();
#endif
}

contracts::Result<void> WindowsSystemAdapter::clear_clipboard() {
#ifdef _WIN32
    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        CloseClipboard();
        return contracts::Result<void>::success();
    }
    return contracts::Result<void>::failure(contracts::ErrorCode::InternalError, "Failed to open clipboard");
#else
    return contracts::Result<void>::success();
#endif
}

contracts::Result<contracts::ScreenCaptureMetadata> WindowsSystemAdapter::capture_screen(uint32_t monitor_id, uint32_t window_id) {
    contracts::ScreenCaptureMetadata meta;
    meta.capture_id = "win_cap_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    meta.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    meta.monitor_id = monitor_id;
    meta.window_id = window_id;
    meta.format = "bmp";
    meta.source = "gdi_screen_capture";

#ifdef _WIN32
    HWND hwnd = (window_id != 0) ? reinterpret_cast<HWND>(static_cast<uintptr_t>(window_id)) : GetDesktopWindow();
    HDC hdcWindow = GetDC(hwnd);
    if (!hdcWindow) {
        return contracts::Result<contracts::ScreenCaptureMetadata>::failure(
            contracts::ErrorCode::InternalError, "Failed to get DC for screen capture"
        );
    }

    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    int width = rcClient.right - rcClient.left;
    int height = rcClient.bottom - rcClient.top;
    if (width <= 0 || height <= 0) {
        width = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);
    }
    meta.width = static_cast<uint32_t>(width);
    meta.height = static_cast<uint32_t>(height);

    HDC hdcMemDC = CreateCompatibleDC(hdcWindow);
    HBITMAP hbmScreen = CreateCompatibleBitmap(hdcWindow, width, height);
    SelectObject(hdcMemDC, hbmScreen);

    // BitBlt screenshot from display to memory DC
    BitBlt(hdcMemDC, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY);

    BITMAP bmpScreen;
    GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);

    BITMAPFILEHEADER bmfHeader{};
    BITMAPINFOHEADER bi{};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmpScreen.bmWidth;
    bi.biHeight = bmpScreen.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;

    meta.image_data.resize(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize);

    char* lpbitmap = reinterpret_cast<char*>(meta.image_data.data() + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
    GetDIBits(hdcWindow, hbmScreen, 0, static_cast<UINT>(bmpScreen.bmHeight), lpbitmap, reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);

    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHeader.bfSize = bmfHeader.bfOffBits + dwBmpSize;
    bmfHeader.bfType = 0x4D42; // "BM"

    memcpy(meta.image_data.data(), &bmfHeader, sizeof(BITMAPFILEHEADER));
    memcpy(meta.image_data.data() + sizeof(BITMAPFILEHEADER), &bi, sizeof(BITMAPINFOHEADER));

    // Save persistent copy for multimodal vision analysis
    std::filesystem::create_directories("scratch");
    std::ofstream out_file("scratch/last_screenshot.bmp", std::ios::binary);
    if (out_file.is_open()) {
        out_file.write(reinterpret_cast<const char*>(meta.image_data.data()), meta.image_data.size());
        out_file.close();
    }

    DeleteObject(hbmScreen);
    DeleteDC(hdcMemDC);
    ReleaseDC(hwnd, hdcWindow);
#else
    meta.width = 1920;
    meta.height = 1080;
#endif

    return contracts::Result<contracts::ScreenCaptureMetadata>::success(meta);
}

contracts::Result<std::vector<contracts::DisplayMetadata>> WindowsSystemAdapter::list_displays() {
#ifdef _WIN32
    DisplayEnumData data;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&data));
    if (!data.displays.empty()) {
        return contracts::Result<std::vector<contracts::DisplayMetadata>>::success(data.displays);
    }
#endif
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
                BOOL muted = FALSE;
                if (SUCCEEDED(pVol->GetMute(&muted))) {
                    s.is_muted = (muted == TRUE);
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

contracts::Result<void> WindowsSystemAdapter::media_play() {
#ifdef _WIN32
    keybd_event(VK_MEDIA_PLAY_PAUSE, 0, 0, 0);
    keybd_event(VK_MEDIA_PLAY_PAUSE, 0, KEYEVENTF_KEYUP, 0);
#endif
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::media_pause() {
#ifdef _WIN32
    keybd_event(VK_MEDIA_PLAY_PAUSE, 0, 0, 0);
    keybd_event(VK_MEDIA_PLAY_PAUSE, 0, KEYEVENTF_KEYUP, 0);
#endif
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::media_stop() {
#ifdef _WIN32
    keybd_event(VK_MEDIA_STOP, 0, 0, 0);
    keybd_event(VK_MEDIA_STOP, 0, KEYEVENTF_KEYUP, 0);
#endif
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::media_next() {
#ifdef _WIN32
    keybd_event(VK_MEDIA_NEXT_TRACK, 0, 0, 0);
    keybd_event(VK_MEDIA_NEXT_TRACK, 0, KEYEVENTF_KEYUP, 0);
#endif
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::media_previous() {
#ifdef _WIN32
    keybd_event(VK_MEDIA_PREV_TRACK, 0, 0, 0);
    keybd_event(VK_MEDIA_PREV_TRACK, 0, KEYEVENTF_KEYUP, 0);
#endif
    return contracts::Result<void>::success();
}

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
#ifdef _WIN32
    if (action == contracts::PowerAction::Lock) {
        LockWorkStation();
        return contracts::Result<void>::success();
    } else if (action == contracts::PowerAction::Sleep) {
        ::system("rundll32.exe powrprof.dll,SetSuspendState 0,1,0");
        return contracts::Result<void>::success();
    } else if (action == contracts::PowerAction::Shutdown) {
        ::system("shutdown /s /t 60 /c \"VANI requested system shutdown\"");
        return contracts::Result<void>::success();
    } else if (action == contracts::PowerAction::Restart) {
        ::system("shutdown /r /t 60 /c \"VANI requested system restart\"");
        return contracts::Result<void>::success();
    }
#else
    (void)action;
#endif
    return contracts::Result<void>::success();
}

contracts::Result<void> WindowsSystemAdapter::send_notification(const contracts::NotificationPayload& notification) {
#ifdef _WIN32
    std::string safe_title = notification.title.empty() ? "VANI Mark 2" : notification.title;
    std::string safe_msg = notification.message;
    std::string toast_cmd = "powershell -NoProfile -Command \"[Windows.UI.Notifications.ToastNotificationManager, Windows.UI.Notifications, ContentType = WindowsRuntime] > $null; $template = [Windows.UI.Notifications.ToastNotificationManager]::GetTemplateContent([Windows.UI.Notifications.ToastTemplateType]::ToastText02); $textNodes = $template.GetElementsByTagName('text'); $textNodes.Item(0).AppendChild($template.CreateTextNode('" + safe_title + "')) > $null; $textNodes.Item(1).AppendChild($template.CreateTextNode('" + safe_msg + "')) > $null; $toast = [Windows.UI.Notifications.ToastNotification]::new($template); [Windows.UI.Notifications.ToastNotificationManager]::CreateToastNotifier('VANI Mark 2').Show($toast);\" 2>nul";
    ::system(toast_cmd.c_str());
#else
    (void)notification;
#endif
    return contracts::Result<void>::success();
}

} // namespace vani::adapters::system
