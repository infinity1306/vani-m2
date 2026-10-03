# VANI Mark 2 — Full Repository Forensic Audit
**Audit Date:** 2026-09-28  
**Operating Environment:** Windows 11 Home Single Language (Build 26100), Intel Core Ultra 7 155H  
**Toolchain:** CMake 4.2.3, GCC 16.2.0 (w64devkit), Ninja / GNU Make 4.4.1, Node.js 18+  
**Local Inference Providers:** Sherpa-ONNX, Piper Neural TTS, Localhost Ollama (qwen2.5:3b)

---

## 1. Executive Summary

VANI Mark 2 is architecturally sound and possesses a working C++20 core, a local-first neural voice loop (Sherpa-ONNX VAD, Zipformer KWS, Whisper Tiny STT, Piper TTS), and a multi-step agent execution layer (AgentController, PolicyEngine, ToolGateway). 

However, deep code inspection reveals substantial **gaps between architectural claims and actual implementation**:
- Several system capability adapters contain fake returns (e.g. fake screen capture metadata without image bytes, simulated browser HTML, empty clipboard/window/media/notification stubs).
- Production runtime contexts default to `InMemory` repositories; cross-session long-term memory is absent.
- The plugin system is a contract-only shell with no discovery or execution engine.
- Self-echo prevention is missing in the voice loop (mic hears speaker output).
- The web gateway server runs mocked telemetry and simulated tasks disconnected from the C++ daemon.

---

## 2. Subsystem Classification Matrix

| Subsystem | Components | Forensic Classification | Real Execution Path vs Stub/Fake |
|---|---|---|---|
| **Audio Capture & Output** | `MiniaudioAudioInput`, `MiniaudioAudioOutput` | **REAL** | Native WASAPI device streaming at 16kHz Float32 input / 22.05kHz output. |
| **Voice Activity Detection** | `RealSileroVADAdapter` | **REAL** | Native Sherpa-ONNX Silero VAD inference on 512-sample frames. |
| **Wake Word Detection** | `SherpaKWSEngine`, `ZipformerKWS` | **REAL** | Streaming neural keyword spotting on "VANI" with pre-roll circular buffer. |
| **Speech-to-Text (STT)** | `SherpaWhisperAdapter`, `SherpaSenseVoiceAdapter` | **REAL** | Offline Sherpa-ONNX Whisper Tiny Multilingual inference on audio segments. |
| **Text-to-Speech (TTS)** | `PiperTTSAdapter`, `WindowsSapiTTSAdapter` | **REAL** | Real Piper neural VITS (`en_US-lessac`) synthesis and Windows SAPI fallback. |
| **Voice Loop & Gating** | `GatedVoicePipeline`, `EndToEndVoiceLoop` | **PARTIAL** | Complete audio-to-text-to-agent pipeline works; **lacks self-echo suppression** while speaking. |
| **Event Bus** | `EventBus` | **REAL** | Multi-threaded worker pool (2 workers, lock-free ring queue, typed event dispatch). |
| **Policy & Permissions** | `PolicyEngine`, `PermissionService` | **REAL** | 5-level risk classification (`Safe`, `Low`, `Medium`, `High`, `Critical`), scoped grants. |
| **Audit & Journal** | `AuditService`, `SystemActionJournal` | **REAL** | Structured audit records of actions, actor IDs, policy decisions, execution durations. |
| **Scheduler** | `Scheduler` | **REAL** | Dedicated background thread with ticking timer (`schedule_at`, `schedule_after`, recurring). |
| **Agent Controller & DAG** | `AgentController`, `ExecutionScheduler`, `PlanValidator` | **REAL** | DAG step scheduling, dependency resolution, topological ordering, timeout enforcement. |
| **Model Provider (Ollama)** | `OllamaAgentModelProvider` | **REAL** | WinHTTP JSON RPC to Ollama (`qwen2.5:3b`) on `127.0.0.1:11434` for planning & replanning. |
| **OS Processes & Terminal** | `ProcessManager`, `TerminalExecutor` | **REAL** | Win32 `CreateProcessA`, `TerminateProcess`, `EnumProcesses`, `_popen`/`_pclose`. |
| **Filesystem Manager** | `FilesystemManager`, `PathSecurity` | **REAL** | Native `std::filesystem` operations with sandbox boundary enforcement. |
| **Audio Volume Control** | `WindowsSystemAdapter::set_volume` | **REAL** | Windows Core Audio COM API (`IAudioEndpointVolume`). |
| **System State Telemetry** | `WindowsSystemAdapter::get_system_state` | **REAL** | Windows `GlobalMemoryStatusEx`, `GetSystemPowerStatus`, `GetUserNameA`. |
| **Screen Capture** | `ScreenCaptureManager`, `WindowsSystemAdapter` | **MOCK / FAKE** | Returns hardcoded metadata (`win_cap_001`, 1920x1080) **without capturing any screen image**. |
| **Webcam Vision** | Vision perception layer | **MISSING** | Flag exists (`requires_camera`), but zero C++ capture implementation. |
| **Browser Automation** | `BrowserManager` | **MOCK / FAKE** | In-memory session IDs; `get_page_content` returns hardcoded dummy HTML; `click`/`type` are stubs. |
| **Windows Management** | `WindowManager`, `WindowsSystemAdapter` | **STUB** | `list_windows` returns empty array `{}`; `focus_window`, `resize_window`, `close_window` are no-ops. |
| **Input Injection** | `InputManager`, `WindowsSystemAdapter` | **STUB** | `inject_key_press`, `inject_mouse_click`, `inject_mouse_move` are empty no-ops. |
| **Clipboard** | `ClipboardManager`, `WindowsSystemAdapter` | **FAKE** | `read_clipboard` returns `""`; `write_clipboard` is a no-op; does not call Win32 clipboard APIs. |
| **Display & Brightness** | `DisplayManager`, `WindowsSystemAdapter` | **FAKE** | Returns hardcoded `\\\\.\\DISPLAY1`, 100% brightness; `set_brightness` is a no-op. |
| **Media Playback Control** | `MediaManager`, `WindowsSystemAdapter` | **STUB** | `media_play`, `pause`, `stop`, `next`, `previous` are empty no-ops; no media key injection. |
| **Power Control** | `PowerManager`, `WindowsSystemAdapter` | **STUB** | `execute_power_action` is a no-op; does not call `ExitWindowsEx` or `LockWorkStation`. |
| **Notifications** | `NotificationManager`, `WindowsSystemAdapter` | **STUB** | `send_notification` is a no-op; no toast or balloon notification dispatched. |
| **Undo System** | System rollback | **STUB** | Only file transaction rollback exists; no unified reversible action registry. |
| **Persistent Memory** | `ShortTermTaskMemory`, Repositories | **PARTIAL** | Task memory exists in RAM; no persistent SQLite or JSON long-term cross-session memory. |
| **Plugin System** | `PluginManifest`, `PluginContext` | **STUB** | Header contracts only; zero plugin discovery, dynamic loading, or IPC runner. |
| **YouTube & Web Tools** | ToolGateway / Capabilities | **MISSING** | No YouTube resolution, news, weather, or multi-mode web search tools in C++. |
| **Remote Gateway & Telemetry**| `vani-gateway`, `gateway_server.cjs` | **PARTIAL / MOCK** | `vani-gateway` C++ is 12-line stub; `gateway_server.cjs` simulates CPU/RAM with `Math.random()`. |

---

## 3. Critical Findings & Architectural Deficiencies

### A. Fake Implementations Identified
1. **Screen Capture**: `WindowsSystemAdapter::capture_screen` lines 534–544 returns a synthetic `ScreenCaptureMetadata` object without executing GDI BitBlt or DXGI Desktop Duplication, returning zero image bytes.
2. **Browser Automation**: `BrowserManager::get_page_content` lines 46–55 returns `"<html><body><h1>Simulated Page</h1><p>Content</p></body></html>"`, and `capture_screenshot` returns 4 dummy bytes `{0x89, 0x50, 0x4E, 0x47}`.
3. **Clipboard**: `WindowsSystemAdapter::read_clipboard` lines 518–523 returns an empty string without touching `OpenClipboard()` / `GetClipboardData()`. `write_clipboard` discards payloads.
4. **Gateway Server Telemetry**: `scripts/gateway_server.cjs` lines 174–180 emits periodic fake telemetry using random numbers rather than reading real host hardware metrics.

### B. Missing Core Capabilities
1. **Voice Self-Echo Guard**: While speaking via Piper TTS, the physical microphone stream remains unmuted and unsuppressed. Utterances from the speaker feed back into VAD and Whisper STT, risking self-trigger loops.
2. **Real Browser Engine**: No real headless browser or Playwright/CDP integration exists.
3. **Win32 OS Driving**: Window management (`EnumWindows`, `SetForegroundWindow`, `ShowWindow`), mouse/keyboard input (`SendInput`), and media key events (`VK_MEDIA_PLAY_PAUSE`) are unimplemented stubs.
4. **Persistent Long-Term Memory**: No storage on disk for user preferences, facts, or cross-session interaction history.
5. **Real Out-of-Process Plugin Engine**: Plugins cannot be placed in a directory, discovered, validated, and loaded.

---

## 4. Phase-by-Phase Remediation Roadmap

1. **Phase 2 Complete**: Build builds cleanly with C++20 MSVC/GCC, 48/49 ctest targets pass.
2. **Phase 4 & 8: Real Win32 System Control & Hardware Adapters**:
   - Implement real GDI/Win32 `capture_screen` (producing real PNG/BMP memory buffer).
   - Implement real Win32 Clipboard (`OpenClipboard`, `GetClipboardData(CF_TEXT)`, `SetClipboardData`).
   - Implement real Win32 Windows Manager (`EnumWindows`, `SetForegroundWindow`, `ShowWindow`, `MoveWindow`).
   - Implement real Win32 Input Injection (`SendInput` for keyboard and mouse).
   - Implement real Win32 Media Keys (`keybd_event` or `SendInput` with `VK_MEDIA_*`).
   - Implement real Win32 Power (`LockWorkStation`, `ExitWindowsEx`).
   - Implement real Win32 Notifications (`Shell_NotifyIconW`).
3. **Phase 5 & 6: Voice Loop Hardening & Self-Echo Guard**:
   - Add microphone gating during TTS playback with acoustic decay cooldown (avoiding self-hearing).
   - Add prompt acknowledgment for long-running agent tasks.
4. **Phase 10: Real Undo System**:
   - Implement `UndoManager` capturing pre-operation state for filesystem operations and settings before mutation.
5. **Phase 11: Real Persistent Memory**:
   - Implement persistent file-backed / SQLite storage for user facts, session continuity, and recall.
6. **Phase 12: Real Plugin Architecture**:
   - Implement directory-based plugin scanner (`plugins/`), manifest parser, and registration.
7. **Phase 13: Vision (Screen & Webcam)**:
   - Wire screen capture buffer into multimodal model requests.
   - Implement Media Foundation / DirectShow webcam frame capture.
8. **Phase 14 & 15: Real Browser & Media/YouTube**:
   - Real browser automation tool (CLI/CDP/Playwright).
   - Media search & YouTube playback handler.
9. **Phase 16 & 17: Web Tools & Reminders**:
   - Real weather, news, search tools using HTTP API.
   - Connect voice/agent to real `Scheduler`.
10. **Phase 19: Real Remote Dashboard & Telemetry**:
    - Update gateway to stream real hardware telemetry from `SystemStateProvider`.
    - Provide token/pairing mechanism.
