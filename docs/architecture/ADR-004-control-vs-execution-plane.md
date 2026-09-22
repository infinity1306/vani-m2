# ADR-004: Control Plane vs. Execution Plane Separation

## Status
Accepted

## Context
If individual tools, agents, or speech models dictate global policy, error states, and permissions, security becomes fragmented and unmaintainable.

## Decision
VANI strictly bifurcates responsibilities into:
1. **Control Plane** (owns task lifecycle, sessions, capability routing, policy, permissions, scheduling, and device coordination).
2. **Execution Plane** (owns speech synthesis, inference, agent steps, browser automation, OCR).
The execution plane decides *how* to execute a capability, while the control plane decides *what* is allowed to happen.

## Consequences
- **Positive**: Strict capability-based security boundaries; agents cannot bypass security policy.
- **Negative**: Adds a clean capability dispatch layer.
