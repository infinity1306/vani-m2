# VANI Mark 2

**Local-First AI Operating Layer for Personal Computing and Connected Hardware**

VANI Mark 2 is an event-driven, local-first, extensible AI operating layer built with modern C++20, real-time audio processing, local STT/TTS models, and a contract-driven modular architecture.

---

## 🏛️ Architecture Overview

```text
┌────────────────────────────────────────────────────────────────────────┐
│                              INTERACTION                               │
│            Voice Streams • Multimodal Text • UI • Devices              │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                        VANI CONTROL PLANE                              │
│  ┌───────────────────────┐  ┌──────────────────────┐  ┌─────────────┐  │
│  │   Lifecycle Manager   │  │   Capability Router  │  │  Policy     │  │
│  └───────────────────────┘  └──────────────────────┘  │  & Security │  │
│  ┌───────────────────────┐  ┌──────────────────────┐  └─────────────┘  │
│  │   Task State Machine  │  │   Asynchronous Bus   │  ┌─────────────┐  │
│  └───────────────────────┘  └──────────────────────┘  │ Health & Obs│  │
│  │   Health Monitor      │  │                      │  └─────────────┘  │
│  └───────────────────────┘  └──────────────────────┘                   │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                         CAPABILITY REGISTRY                            │
│  Tools (Terminal, FS, Git) • Agents (Odysseus, Hermes) • Models • Dev  │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                        REPLACEABLE ADAPTERS                            │
│  Sherpa / Whisper • Ollama / llama.cpp / Gemini • In-Memory / SQLite   │
└────────────────────────────────────────────────────────────────────────┘
```

### Dependency Flow

```text
apps/  -->  runtime/  -->  services/  -->  contracts/
                               ^
adapters/ --------------------| (implements contracts)
```

---

## 🚀 Key Features

- **Local-First & Offline Capable**: Zero-latency local inference with Sherpa-ONNX, Piper TTS, and Whisper / Zipformer STT.
- **Contract-First Architecture**: Strictly decoupled interfaces under `contracts/` with clean implementations in `adapters/` and `capabilities/`.
- **Real-Time Voice Pipeline**: End-to-end voice loop with adaptive VAD, streaming ASR, dedicated wake-word detection, and low-latency audio output via miniaudio.
- **Extensible Capabilities**: System tool execution (filesystem watcher, application registry, process execution) guarded by policy and capability permissions.
- **Observable & Benchmark-Tested**: Comprehensive benchmark suites and validation reports for latency, accuracy, and memory footprint.

---

## 📁 Repository Structure

- `adapters/` - Hardware and engine adapters (Sherpa-ONNX, Piper TTS, LLM backends)
- `apps/` - Application entry points and runtime hosts
- `audio/` - Real-time audio input/output, VAD, ring buffering, and miniaudio integration
- `benchmarks/` - Synthetic and real voice corpora, latency benchmarks
- `capabilities/` - Pluggable capabilities (System, Filesystem, Terminal, App Registry)
- `config/` - Configuration management
- `contracts/` - Core architectural contracts, types, and event definitions
- `data/` - Audio device manifests and configuration data
- `docs/` - Architecture Decision Records (ADRs), blueprints, and phase validation audits
- `observability/` - Metrics, telemetry, logging, and health checking
- `runtime/` - Event bus, lifecycle manager, task state machine, execution router
- `scripts/` - Gateway servers, evaluation scripts, and verification runners
- `src/` - Web frontend / dashboard interface
- `voice/` - Voice orchestrator, wake-word engine, and conversation loops

---

## 🛠️ Build & Requirements

### Prerequisites
- C++20 compatible compiler (MSVC 2022 / Clang / GCC)
- CMake 3.20+
- Ninja build system
- Node.js 18+ (for frontend dashboard)

### Building the C++ Engine
```powershell
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Running the Web Dashboard
```powershell
npm install
npm run dev
```

---

## 📄 License
Proprietary / All rights reserved.
