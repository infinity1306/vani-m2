# ADR-010: Out-of-Process Sandboxed Plugin Architecture

## Status
Accepted

## Context
Third-party extensions, custom enterprise integrations, and experimental tools must not compromise the stability, memory space, or security of VANI Core.

## Decision
Plugins declare capabilities, tools, and required permissions via a static `PluginManifest`. Plugins run out-of-process in isolated sandboxes and register tools/agents through the standard `PluginContext` API. Plugins never gain direct pointer access to VANI Core runtime memory or internal class structures.

## Consequences
- **Positive**: Fault tolerance (crashed plugins do not bring down VANI); strict capability isolation and security guardrails.
