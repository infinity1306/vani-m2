# ADR-007: Capability-Based Permission Model

## Status
Accepted

## Context
Traditional role-based access control or binary "admin/user" permissions are inadequate for autonomous agents executing complex CLI, filesystem, and network operations.

## Decision
Permissions in VANI are capability-scoped (e.g. `filesystem.read`, `filesystem.write`, `terminal.execute`, `microphone.use`, `github.write`). Every tool and agent explicitly declares required permissions in its manifest. The Policy Engine evaluates risk level, execution context, and user presence to determine whether to allow, sandbox, or prompt for confirmation.

## Consequences
- **Positive**: Principle of least privilege enforced across all agent steps.
- **Negative**: Agents must specify granular permission requirements.
