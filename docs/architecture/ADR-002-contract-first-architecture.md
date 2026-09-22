# ADR-002: Contract-First Architecture

## Status
Accepted

## Context
AI model providers, speech-to-text engines, agent frameworks, and device hardware protocols change rapidly. Directly binding the VANI runtime to specific third-party tools (e.g. Ollama, Whisper, Odysseus) creates brittle technical debt and vendor lock-in.

## Decision
All subsystem interactions must pass through abstract, stable, versioned pure virtual interfaces (`STTEngine`, `ModelProvider`, `Agent`, `Tool`, `Device`, `MemoryProvider`). The Core Runtime never depends directly on vendor implementations. Adapters implement contracts, and Capabilities consume contracts.

## Consequences
- **Positive**: Complete replaceability. Replacing Sherpa with Whisper, or Ollama with a new local engine, requires zero edits to the VANI Task Manager, Event Bus, or UI.
- **Negative**: Requires interface discipline and upfront type modelling.
