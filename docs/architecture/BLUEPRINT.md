# VANI Mark 2 — Architecture Blueprint & Core Foundation

## 1. System Overview

VANI Mark 2 is an **event-driven, local-first, extensible AI operating layer** for personal computing and connected hardware.

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
│                                                       └─────────────┘  │
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

---

## 2. Dependency Direction Rules

```text
apps/  -->  runtime/  -->  services/  -->  contracts/
                              ^
adapters/ --------------------| (implements contracts)
capabilities/ ----------------| (consumes contracts)
```

1. **Rule 1**: The Core Runtime depends only on abstract contracts.
2. **Rule 2**: The Capability Router matches requested capabilities to available implementations dynamically — no hardcoded vendor names.
3. **Rule 3**: All tasks must transition through the `TaskManager` state machine (`CREATED` -> `PLANNING` -> `RUNNING` -> `COMPLETED`/`FAILED`/`CANCELLED`).
4. **Rule 4**: Privileged actions pass through `PolicyEngine` and `PermissionService` capability evaluation.
5. **Rule 5**: Operations support cooperative cancellation through `CancellationToken`.

---

## 3. Module Responsibilities Matrix

| Directory | Core Purpose | Dependency Policy |
| :--- | :--- | :--- |
| `contracts/` | Stable, versioned abstract interfaces (`Result<T>`, `Task`, `Tool`, `Agent`, `ModelProvider`, `STTEngine`, `Device`) | Zero external dependencies |
| `runtime/` | Control plane: state machines, event bus, capability routing, policy, permissions, task orchestration | Depends only on `contracts/` |
| `adapters/` | External connector implementations (Sherpa, Whisper, Ollama, Gemini, Odysseus, Hermes, SQLite) | Implements `contracts/` |
| `observability/` | Structured JSON logging, health checks, latency tracing, diagnostics | Independent |
| `config/` | Tiered schema-validated profile loader | Independent |
| `apps/` | Executable targets (`vani-runtime`, `vani-cli`, `vani-gateway`) | Integrates runtime & adapters |
| `tests/` | Unit, Contract, Integration, Security, and Architecture boundary test suites | Verifies compliance |

---

## 4. Demonstrating Future Extensibility

To add any new capability:
1. **New STT Engine**: Create an adapter inheriting from `vani::contracts::STTEngine` and register with runtime. Zero edits to `TaskManager` or `EventBus`.
2. **New LLM Provider**: Create an adapter inheriting from `vani::contracts::ModelProvider`. Zero edits to routing logic.
3. **New Coding/Research Agent**: Create an adapter inheriting from `vani::contracts::Agent`.
4. **New Connected Device**: Implement `vani::contracts::Device`.
