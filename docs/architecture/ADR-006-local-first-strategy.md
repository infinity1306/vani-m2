# ADR-006: Local-First Operating Model & Offline Resilience

## Status
Accepted

## Context
VANI is an operating layer for personal computing. Loss of internet connectivity must never render local tools, audio processing, memory, or local LLMs non-functional.

## Decision
Local-first is a first-class architectural state. Capabilities explicitly declare `requires_network` and `requires_cloud`. In offline mode, the capability router transparently routes requests to local providers (Ollama / llama.cpp / local Whisper / local rule heuristics), pausing cloud-dependent features with clear degradation explanations.

## Consequences
- **Positive**: Total system availability regardless of internet connectivity; privacy preservation for sensitive data.
