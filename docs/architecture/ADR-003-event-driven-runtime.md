# ADR-003: Asynchronous Event-Driven Runtime

## Status
Accepted

## Context
VANI coordinates concurrent perception streams, voice audio chunk buffers, background agent processes, and UI updates. Synchronous blocking calls degrade voice responsiveness and user experience.

## Decision
VANI uses an asynchronous, thread-safe Event Bus with typed subscriptions, correlation IDs, task/session associations, and backpressure control. The Event Bus is an explicitly owned subsystem of `VaniRuntime`, avoiding uncontrolled global singletons.

## Consequences
- **Positive**: Complete decoupling between event producers and consumers; linear trace auditability via correlation IDs.
- **Negative**: Requires event versioning discipline.
