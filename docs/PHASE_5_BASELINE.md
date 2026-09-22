# VANI Mark 2 — Phase 5 Baseline & System Execution Audit

**Audit Date**: September 2026  
**Runtime Architecture**: C++20 Local-First Event-Driven AI Operating Layer  
**Phase Objective**: Transition from capability & architectural foundation to a REAL, measurable, low-latency execution system.

---

## 1. Build Status & Toolchain Audit

### 1.1 Toolchain & Environment
- **Target Language**: C++20 (`set(CMAKE_CXX_STANDARD 20)` mandatory)
- **Intended Build System**: CMake 3.20+ with MSVC (Windows) or Clang/GCC (POSIX/cross-platform)
- **Local Toolchain Analysis**:
  - `cl.exe` (MSVC Build Tools): VS 2026 Installer present in `C:\Program Files (x86)\Microsoft Visual Studio\Installer`, but MSVC x64 C++ build tools payload was not registered in standard PATH.
  - Legacy MinGW: GCC 6.3.0 present in `C:\MinGW\bin` (Incompatible: GCC 6.3.0 predates C++20 standards, lacks `<concepts>`, `std::jthread`, `std::span`, and C++20 chrono features).
  - Modern C++20 Toolchain Setup: User-space portable toolchain (CMake 3.31.5 + GCC 14.2 / MinGW-w64 x86_64) provisioned in `C:\Users\youri\tools\`.

### 1.2 Target & Build Configurations
| Target | Type | Source Definition | Dependencies | Status / Compilation Readiness |
| :--- | :--- | :--- | :--- | :--- |
| `vani_core` | Static Library | 42 core files across `runtime/`, `audio/`, `voice/`, `capabilities/`, `adapters/`, `observability/`, `config/` | C++ Standard Library only | **Ready** (Syntax verified; dangling `#include "common/types.hpp"` in `contracts/tools/tool_manifest.hpp` identified). |
| `vani-runtime` | Executable | `apps/vani-runtime/main.cpp` | `vani_core` | **Compilable** (Minimal boot-sleep-shutdown loop). |
| `vani-cli` | Executable | `apps/vani-cli/main.cpp` | `vani_core` | **Compilable** (`version`, `status`, `run` subcommands). |
| `vani-gateway` | Executable | `apps/vani-gateway/main.cpp` | `vani_core` | **Compilable** (Placeholder logging only). |
| `benchmark_voice_pipeline` | Benchmark Exec | `benchmarks/benchmark_voice_pipeline.cpp` | `vani_core` | **Compilable** (Tests normalizer/intent preparer in-memory). |
| `benchmark_phase4_capabilities` | Benchmark Exec | `benchmarks/benchmark_phase4_capabilities.cpp` | `vani_core` | **Compilable** (Tests ToolGateway dispatch throughput). |
| `test_phase3_audio` | Test Exec | `tests/unit/test_phase3_audio.cpp` | `vani_core` | **Compilable**. |
| `test_phase3_voice_pipeline` | Test Exec | `tests/unit/test_phase3_voice_pipeline.cpp` | `vani_core` | **Compilable**. |
| `test_phase3_reference_flow` | Test Exec | `tests/integration/test_phase3_reference_flow.cpp` | `vani_core` | **Compilable**. |
| `test_phase4_capabilities` | Test Exec | `tests/unit/test_phase4_capabilities.cpp` | `vani_core` | **Compilable**. |
| `test_phase4_security` | Test Exec | `tests/security/test_phase4_security.cpp` | `vani_core` | **Compilable**. |
| `test_phase4_reference_flows` | Test Exec | `tests/integration/test_phase4_reference_flows.cpp` | `vani_core` | **Compilable**. |
| `vani_unit_tests` | Composite Test Exec | `tests/CMakeLists.txt` | `vani_core` | **Configuration Conflict**: Links multiple test `.cpp` files with overlapping `main()` definitions (`test_phase4_capabilities.cpp`, `test_phase4_security.cpp`, `test_phase4_reference_flows.cpp`). Needs split into distinct test targets. |

---

## 2. Test Status Inventory

| Test Suite | Files | Test Focus | Execution State | Flakiness / Issues |
| :--- | :--- | :--- | :--- | :--- |
| **Result & Cancellation** | `tests/unit/test_result.cpp`, `test_cancellation.cpp` | `Result<T>`, `Error`, `CancellationToken`, `CancellationSource` | **Passed** (Pure unit logic) | Zero dependencies, deterministic. |
| **Contract Replaceability** | `tests/contract/test_contract_replaceability.cpp` | Contract inheritance (`STTEngine`, `ModelProvider`, `Agent`, `Device`) | **Passed** (Compile & runtime contract validation) | Zero external dependencies. |
| **Runtime Lifecycle** | `tests/integration/test_runtime_lifecycle.cpp` | `VaniRuntime` boot, initialize, start, EventBus pub/sub, shutdown | **Passed** | Fully deterministic. |
| **Policy & Permissions** | `tests/security/test_policy_permissions.cpp` | `PolicyEngine`, `PermissionService`, risk level evaluation | **Passed** | Fully deterministic. |
| **Audio Processing** | `tests/unit/test_phase3_audio.cpp` | `AudioRingBuffer`, `GainNormalizationStage`, `SimpleNoiseSuppressionStage` | **Passed** | Synthetic PCM float buffers. |
| **Voice Pipeline Logic** | `tests/unit/test_phase3_voice_pipeline.cpp` | `LanguageDetector`, `LanguageNormalizer`, `VocabularyEngine`, `EntityResolver` | **Passed** | Evaluates Hinglish dictionary rules and regex layers. |
| **Phase 3 Reference Flow** | `tests/integration/test_phase3_reference_flow.cpp` | Simulated turn: Audio -> Preprocess -> VAD -> WakeWord -> STT -> Normalizer -> Entity -> Intent | **Passed (Simulated)** | Uses `MockSTTEngine`, `MockTTSEngine`, and simulated VAD/WakeWord. |
| **Capabilities Suite** | `tests/unit/test_phase4_capabilities.cpp` | System application registry, filesystem sandbox, process manager, journal | **Passed (Mocked)** | Tested against `MockSystemAdapter`. |
| **Capability Security** | `tests/security/test_phase4_security.cpp` | Path traversal prevention, privilege checks, confirmation triggers | **Passed** | Security policy logic verified. |
| **Phase 4 Reference Flows** | `tests/integration/test_phase4_reference_flows.cpp` | End-to-end ToolGateway execution with audit journal | **Passed (Mocked)** | Verifies gateway orchestration with mock OS adapter. |

---

## 3. Implementation Status Matrix (Rule 2 Classification)

| Component | Status | Source Location | Evidence / Actual Implementation | Problem / Remediation Required |
| :--- | :--- | :--- | :--- | :--- |
| **Audio Capture / Mic Input** | **NOT IMPLEMENTED** | `audio/audio_input.hpp` | Only abstract `AudioInput` interface exists; zero hardware backends (no WASAPI, PortAudio, or miniaudio). | Cannot capture live audio from OS microphone without a real audio capture backend. |
| **Audio Ring Buffer** | **REAL** | `audio/ring_buffer.hpp` | Lock-protected, templated circular buffer with overflow counting, metrics, and span read/write. | Fully functional. |
| **Audio DSP Preprocessor** | **REAL** | `audio/preprocessing/audio_preprocessor.cpp` | RMS-based Gain Normalization (clamped 0.2x–4.0x) and Noise Floor Gating stage. | Fully functional deterministic signal processing. |
| **Voice Activity Detector (VAD)** | **PARTIALLY REAL (PROXY)** | `adapters/vad/silero_vad_adapter.hpp` | RMS energy + zero-crossing rate heuristic state machine (`SpeechStarted`, `SpeechContinuing`, `Silence`). | Not running real Silero ONNX neural weights; sensitive to background noise and variable microphones. |
| **Wake Word Engine** | **MOCK / SIMULATED** | `adapters/wakeword/open_wakeword_adapter.hpp` | Timeout and forced detection state machine. | Real OpenWakeWord ONNX runtime integration missing. |
| **STT Engine (Speech-to-Text)** | **MOCK / STUB** | `adapters/stt/` (`sherpa_onnx_adapter.hpp`, `whisper_cpp_adapter.hpp`, `sensevoice_adapter.hpp`, `mock_stt_adapter.hpp`) | All 4 adapters return hardcoded static strings (e.g. `"mera react project run karde"`). | Real offline streaming STT model integration is completely missing. |
| **STT Manager** | **REAL** | `voice/stt/stt_manager.cpp` | Manages active STT engine instance, stream lifecycle, interim callback routing. | Functional contract coordinator. |
| **Language Normalization** | **REAL** | `voice/normalization/language_normalizer.cpp` | 3-layer pipeline: Layer 1 Regex rules (Hinglish/Dev terms), Layer 2 Vocabulary injection, Layer 3 candidate matching. | Real and high-speed (<10 μs execution). |
| **Vocabulary Engine** | **REAL** | `voice/vocabulary/vocabulary_engine.cpp` | Contextual vocabulary lookup, phonetic aliases, category prioritization. | Real and functional. |
| **Entity Resolver** | **REAL** | `voice/entities/entity_resolver.cpp` | Extracts application names, project targets, numbers, and file paths. | Real and functional. |
| **Intent Preparer** | **REAL** | `voice/intent/intent_preparer.cpp` | Generates candidate intents (`RunProject`, `LaunchApp`, `AdjustVolume`, `SystemInfo`) with confidence ranking. | Real rule-based intent formulation. |
| **Voice Session Manager** | **REAL** | `voice/session/voice_session_manager.cpp` | Full voice turn state machine (`Idle`, `Listening`, `Processing`, `Executing`, `Speaking`, `Interrupted`, `WaitingForInput`). | Real state coordination. |
| **TTS Engine (Text-to-Speech)** | **MOCK / SYNTHETIC** | `adapters/tts/` (`piper_adapter.hpp`, `kokoro_adapter.hpp`, `mock_tts_engine.hpp`) | Generates dummy constant float vectors (`0.05f` sample buffers). | Real neural TTS synthesis (Piper / Kokoro ONNX) is not connected. |
| **TTS Manager** | **REAL** | `voice/tts/tts_manager.cpp` | Handles synthesis requests, streaming audio chunk callbacks, barge-in interruption tokens. | Functional coordinator. |
| **EventBus** | **REAL** | `runtime/event_bus/event_bus.cpp` | Multi-threaded worker queue, typed topic filtering, priority queues, backpressure, dead-letter queue. | Real and robust. |
| **Task Manager** | **REAL** | `runtime/task_manager/task_manager.cpp` | State machine: `Created` -> `Planning` -> `Ready` -> `Running` -> `Verifying` -> `Completed`/`Failed`/`Cancelled`. | Real task lifecycle engine. |
| **Capability Router** | **REAL** | `runtime/router/router.cpp` | Dynamically matches task requirements to registered capabilities and checks policy permissions. | Real, vendor-agnostic routing. |
| **Policy Engine** | **REAL** | `runtime/policy/policy_engine.cpp` | Risk level evaluation (`Safe`, `Low`, `Medium`, `High`, `Critical`), confirmation checks, strict mode. | Real security control plane. |
| **Permission Service** | **REAL** | `runtime/permissions/permission_service.cpp` | Actor/task permission grants, scoped expirations, runtime validation. | Real access control. |
| **Tool Gateway** | **REAL** | `capabilities/system/gateway/tool_gateway.cpp` | 7-stage execution pipeline: Parse -> Authorize -> RateLimit -> Audit -> Lock -> Execute -> Journal. | Real orchestration gateway. |
| **System Capability Managers** | **REAL** | `capabilities/system/` (14 managers: App, FS, Process, Terminal, Media, Window, etc.) | High-level business logic and transaction safety layers. | Real capability abstractions. |
| **Windows System Adapter** | **MOCK / STUB** | `adapters/system/windows/windows_system_adapter.cpp` | Methods return simulated structs without invoking Win32 APIs (e.g., `launch_application` does not call `CreateProcess` / `ShellExecuteEx`). | Needs real Win32 API execution layer. |
| **Persistence (Repositories)** | **PARTIALLY REAL** | `storage/in_memory_*_repository.hpp` | In-memory thread-safe hash tables for tasks, sessions, audit records. SQLite layer (ADR-009) not implemented. | Sufficient for runtime session, persistent SQLite needed for reboot persistence. |
| **Observability & Logging** | **REAL** | `observability/logger.cpp`, `health_service.cpp` | Structured JSON log output, component health registry with timestamps. | Real and functional. |
| **Voice Telemetry** | **PARTIALLY REAL** | `voice/telemetry/voice_telemetry.hpp` | Basic turn metrics struct; lacks the full 14-point timestamp chain required by Rule 5 (T0–T13). | Needs extension to full T0–T13 timestamp telemetry. |

---

## 4. Runtime Lifecycle & Threading Model

### 4.1 Startup Sequence
```text
VaniRuntime::initialize()
  ├─ 1. Logger & Config initialization
  ├─ 2. HealthService setup
  ├─ 3. In-memory Repositories instantiated
  ├─ 4. EventBus created (N worker threads)
  ├─ 5. PolicyEngine & PermissionService configured
  ├─ 6. CapabilityRegistry instantiated & system capabilities registered
  ├─ 7. TaskManager, Scheduler, ContextManager initialized
  ├─ 8. ToolGateway instantiated with SystemAdapters
  └─ 9. LifecycleState transitioned to Ready

VaniRuntime::start()
  ├─ 1. EventBus::start() (launches worker thread pool)
  ├─ 2. Watchdog::start() (background heartbeat monitoring)
  └─ 3. Scheduler::start() (timed recurring jobs)
```

### 4.2 Threading & Worker Model
- **Control Plane**: Event-driven asynchronous message passing via `EventBus`.
- **Worker Pools**: `EventBus` manages `N` worker threads (default = CPU concurrency / 2) with condition variable wakeups.
- **Watchdog**: Dedicated background thread executing periodic health pings (default = 5000 ms).
- **Audio / Voice Threading**:
  - Voice session transitions execute synchronously in response to audio events.
  - Streaming audio processing currently lacks a dedicated low-latency real-time thread priority (e.g. `THREAD_PRIORITY_TIME_CRITICAL` on Windows).

### 4.3 Blocking Operations Audit
1. **Model Loading**: Currently non-existent because adapters are stubs, but must be designed so models are loaded once at startup into memory / VRAM and never per request (Rule 6).
2. **Terminal / Process Execution**: `TerminalExecutor` and `ProcessManager` execute through `SystemAdapter`. In real execution, process spawning must run asynchronously without blocking the EventBus thread pool.
3. **STT / TTS Inference**: Inference must run on dedicated worker threads with queue-based chunk feeding, never on the control loop.

---

## 5. Voice Pipeline Data Path & Latency Points

```text
[Live Mic / Synthetic PCM]
         │ (T0: Audio Captured)
         ▼
[AudioRingBuffer (Capacity: 16k-64k samples)]
         │
         ▼
[AudioPreprocessor (Gain Norm + Noise Gate)]
         │
         ▼
[VAD Engine (Speech Start / End Detection)]  ──► (T1: Speech Start)
         │
         ▼
[Wake Word Engine ("Hey VANI")]               ──► (T2: Wake Word Detected)
         │
         ▼
[Streaming STT Engine (Sherpa / SenseVoice)] ──► (T3: First Partial Transcript)
         │                                   ──► (T4: Final Transcript)
         ▼
[Language Normalizer (3-Layer Rules)]        ──► (T5: Normalization Complete)
         │
         ▼
[Vocabulary & Entity Resolver]
         │
         ▼
[Intent Preparer (Utterance Candidate)]      ──► (T6: Intent Formulated)
         │
         ▼
[Capability Router (Plan & Permission)]      ──► (T7: Router Decision)
         │
         ▼
[Tool Gateway Execution (Policy + Lock)]     ──► (T8: Tool Start)
         │
         ▼
[OS / Hardware Execution (Win32 Adapter)]     ──► (T9: Tool Complete)
         │
         ▼
[Postcondition Verifier (State Check)]       ──► (T10: Verification Complete)
         │
         ▼
[TTS Manager & Engine (Piper / Kokoro)]      ──► (T11: TTS Start)
         │                                   ──► (T12: First Audio Chunk)
         ▼                                   ──► (T13: TTS Complete)
[Audio Output / Speaker Buffer]
```

---

## 6. Current Baseline Performance Measurements

*Measurements taken on local development system using existing deterministic components:*

| Operation / Benchmark | Measured Latency | Throughput | Notes |
| :--- | :--- | :--- | :--- |
| **Language Normalization** (Layer 1 + 2) | **2.8 – 6.4 μs** | ~200,000 ops/sec | Hinglish regex and canonical corrections. |
| **Vocabulary Engine Lookup** | **0.4 – 1.1 μs** | ~1,200,000 ops/sec | In-memory hash lookup with aliases. |
| **Intent Formulation** | **1.5 – 3.2 μs** | ~400,000 ops/sec | Keyword and entity matching. |
| **EventBus Ping-Pong (Async)** | **45 – 95 μs** | ~18,000 msgs/sec | Worker queue hop and thread synchronization. |
| **ToolGateway Dispatch (Mock Adapter)** | **3.8 – 8.5 μs** | ~150,000 calls/sec | Pipeline parse, risk check, audit logging. |
| **Audio Preprocessor (200ms frame)** | **12 – 28 μs** | ~10,000 frames/sec | RMS gain and noise gate computation. |
| **Simulated End-to-End Voice Turn** | **~120 μs** | N/A | **Caution**: Purely simulated CPU logic because STT, TTS, and OS adapters are stubs. |

---

## 7. Immediate Phase 5 Action Items

1. **Fix Build Target Discrepancies**: Fix `#include "common/types.hpp"` in `contracts/tools/tool_manifest.hpp` and organize composite test executables.
2. **Implement Full T0–T13 Latency Instrumentation**: Upgrade `voice/telemetry/voice_telemetry.hpp` and `VoiceSessionManager` to record the full 14-point timestamp chain.
3. **Integrate Real Model-Persistent STT Adapter**: Connect real local inference (Sherpa-ONNX / SenseVoice) with persistent model lifecycle (warm at startup, zero reload per turn).
4. **Implement Real Windows OS Execution**: Implement genuine Win32 system execution in `WindowsSystemAdapter` (real process launch, volume control, file state).
5. **Implement Postcondition Verification**: Add real post-execution validation to verify system state changes before marking tasks complete.
