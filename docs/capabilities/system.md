# System Capability Layer Architecture

## 1. Overview
The VANI Mark 2 System Capability Layer provides a stable, unified, capability-based abstraction over host operating-system facilities while strictly isolating platform-specific implementations behind platform adapters.

Voice commands, agents, models, and plugins never gain direct access to raw OS handles, shell execution, or unrestricted filesystems. Every system interaction passes through the VANI 10-Step Tool Execution Pipeline.

```text
User / Agent Intent
        ↓
VANI Runtime
        ↓
Capability Router / Gateway
        ↓
Policy Engine
        ↓
Permission Service
        ↓
Resource Limit Check
        ↓
Platform Adapter / Executor
        ↓
Postcondition Verification
        ↓
Action Journal & Audit
        ↓
Result
```

## 2. Capability Catalog

| Capability ID | Category | Risk Level | Reversibility | Idempotency | Default Permission |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `system.application.open` | Application | Medium | Reversible | NonIdempotent | `system.application.open` |
| `system.application.close` | Application | Medium | PartiallyReversible | Idempotent | `system.application.close` |
| `system.application.focus` | Application | Low | Reversible | Idempotent | `system.application.focus` |
| `system.process.list` | Process | Low | Reversible | Idempotent | None (Public) |
| `system.process.stop` | Process | Medium | PartiallyReversible | Idempotent | `system.process.stop` |
| `system.process.terminate` | Process | High | Irreversible | Idempotent | `system.process.terminate` |
| `filesystem.read` | Filesystem | Low | Reversible | Idempotent | `filesystem.read` |
| `filesystem.write` | Filesystem | Medium | Reversible (TX) | NonIdempotent | `filesystem.write` |
| `filesystem.delete` | Filesystem | High | Reversible (Trash) | Idempotent | `filesystem.delete` |
| `terminal.execute` | Terminal | High | PartiallyReversible | NonIdempotent | `terminal.execute` |
| `browser.open` | Browser | Low | Reversible | NonIdempotent | `browser.access` |
| `browser.navigate` | Browser | Low | Reversible | Idempotent | `browser.access` |
| `window.focus` | Window | Low | Reversible | Idempotent | `window.control` |
| `clipboard.read` | Clipboard | Medium | Reversible | Idempotent | `clipboard.read` |
| `clipboard.write` | Clipboard | Low | Reversible | NonIdempotent | `clipboard.write` |
| `screen.capture` | Screen | Medium | Reversible | Idempotent | `screen.capture` |
| `media.volume` | Media | Low | Reversible | Idempotent | `media.control` |
| `system.get_state` | System State | Low | Reversible | Idempotent | None (Public) |
| `system.lock` | Power | Medium | Reversible | Idempotent | `system.power` |
| `system.shutdown` | Power | Critical | Irreversible | NonIdempotent | `system.power` + Confirm |

## 3. Fast Path vs Agent Path
- **Fast Path**: Deterministic commands (`open Chrome`, `volume 50`, `take screenshot`, `battery status`) are dispatched directly via `ToolGateway::execute_fast_path` within < 1 ms, bypassing large language models.
- **Agent Path**: Complex or multi-step intents are dispatched by agents through the exact same 10-step Tool Gateway.
