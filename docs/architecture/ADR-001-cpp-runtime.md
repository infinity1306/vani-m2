# ADR-001: Modern C++20 as Core Runtime Language

## Status
Accepted

## Context
VANI Mark 2 requires sub-millisecond event loop responsiveness, deterministic resource allocation, direct OS/audio hardware access (LE Audio, WASAPI, CoreAudio, ALSA), and zero-cost abstraction layers for native execution on local hardware (NPU/GPU/CPU). Higher-level runtime environments (e.g. standard Node.js or Python runtimes) introduce non-deterministic garbage collection pauses, GIL locks, and high base memory consumption.

## Decision
The core VANI Control Plane runtime is implemented in standard ISO C++20 using strict RAII, smart pointers, value semantics, and thread-safe asynchronous worker pools.

## Consequences
- **Positive**: Sub-millisecond IPC loopback latency, deterministic memory footprints (<30MB baseline), native hardware acceleration hooks.
- **Negative**: Requires strict compilation hygiene and contract definition.
