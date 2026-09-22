# ADR-008: Versioned Contracts and Event Schemas

## Status
Accepted

## Context
As VANI evolves over multiple years, public internal interfaces (tools, events, agents, tasks) will undergo iterations. Unversioned contracts lead to silent runtime deserialization failures and backward incompatibility.

## Decision
Every public contract and event schema embeds a `SemanticVersion` (e.g. `event.v1`, `tool.v1`, `agent.v1`). Breaking changes increment the major version number. Adapters and listeners declare compatible version ranges.

## Consequences
- **Positive**: Long-term binary and IPC stability; seamless rolling upgrades of worker processes.
