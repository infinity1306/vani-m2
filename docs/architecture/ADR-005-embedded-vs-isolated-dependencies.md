# ADR-005: Embedded vs. Isolated Dependency Policy

## Status
Accepted

## Context
A monolithic single-process binary crashes if an untrusted plugin or complex external Python runtime segfaults. Conversely, turning VANI into a 30-microservice Docker web introduces unacceptable IPC overhead and latency.

## Decision
- **Embedded Dependencies**: Small, stable, native C++ libraries (SQLite, nlohmann/json, tree-sitter, crypto) run in-process for zero-overhead performance.
- **Isolated Dependencies**: Large, volatile, crash-prone external tools (browser automation, untrusted third-party plugins, heavy Python agent runtimes) run out-of-process in sandboxed worker processes communicating via typed IPC.

## Consequences
- **Positive**: High speed for core OS operations combined with fault isolation for external extensions.
