# PHASE 7 — CONTROLLED AGENTIC EXECUTION LAYER ARCHITECTURE

**System:** VANI Mark 2 — Advanced Autonomous On-Device Voice Assistant  
**Subsystem:** Controlled Agent Runtime & Bounded Planner/Executor  
**Specification Version:** 1.0.0  
**Status:** VALIDATED  

---

## 1. Architectural Philosophy & Mission

Phase 6E established a frozen, production-grade end-to-end voice loop on physical hardware (WASAPI capture → Silero VAD → Sherpa KWS → Whisper Tiny STT → Normalizer → Tool Gateway → Win32 Adapter → Piper TTS → WASAPI playback).

Phase 7 adds a **controlled agentic execution layer** capable of handling multi-step, ambiguous, contextual, and goal-oriented tasks without introducing unrestricted autonomy or uncontained multi-agent swarms.

### Core Architectural Invariant
> **THE AGENT CAN PLAN, BUT THE AGENT CANNOT GRANT ITSELF AUTHORITY.**

Every actual capability execution continues strictly through:
```text
User Request / Voice Turn
          ↓
  Complexity Router
    ↙           ↘
FAST PATH      AGENT
   ↓              ↓
   └──────┬───────┘
          ↓
    PolicyEngine (ALLOW / DENY / REQUIRE_CONFIRMATION)
          ↓
     ToolGateway (10-Step Invocation Pipeline)
          ↓
      Capability (Win32 OS, Browser, Terminal, Filesystem)
          ↓
  Postcondition Verification (Independent Host Snapshot Check)
          ↓
   Audit Journal & Telemetry
```

The agent is a planner and dependency-aware orchestrator. It does **not** possess special OS privileges, cannot bypass the `PolicyEngine`, cannot bypass `ToolGateway`, and cannot trust its own assertions without independent empirical verification.

---

## 2. Non-Goals & Strict Safety Invariants

To guarantee user safety and determinism, Phase 7 enforces explicit non-goals:
1. **No Unrestricted Autonomous Computer Control**: The agent cannot execute arbitrary shell commands or take over the desktop without explicit tool wrappers and policy consent.
2. **No Multi-Agent Swarms**: A single controlled agent with bounded DAG scheduling is employed; no recursive or unmonitored subagent spawning.
3. **No Self-Modifying Code**: The agent cannot modify runtime binaries, contracts, or policies.
4. **No Privilege Escalation**: The agent cannot reconfigure `PolicyEngine` rules, modify user roles, or grant itself `SystemSafety` priority.
5. **No Secret Ingestion**: Passwords, bearer tokens, API keys, and private credentials are automatically scrubbed before entering task memory or audit journals.
6. **No Unbounded Loops**: Step retries (max 2), replans (max 3), step timeouts (30s), and global timeouts (300s) are bounded by immutable resource budgets.

---

## 3. Fast Path vs. Agent Path (Deterministic Complexity Classifier)

Simple discrete commands should never incur LLM planning overhead or latency. The `ComplexityClassifier` employs deterministic heuristics to partition incoming commands:

```text
Voice Transcript: "Chrome kholo"
→ Route: FAST_PATH (sub-millisecond dispatch via ToolGateway::execute_fast_path)

Voice Transcript: "Chrome kholo aur Google open karo"
→ Route: AGENT_PATH (Multi-step conjunction "aur" / "and" triggers DAG planner)

Voice Transcript: "Agar Chrome open hai toh close karo"
→ Route: AGENT_PATH (Conditional logic trigger "agar" / "if" triggers agent)

Voice Transcript: "Desktop par files identify karke summary bana"
→ Route: AGENT_PATH (Workflow trigger "identify karke" triggers multi-step planner)
```

Both routes merge into `PolicyEngine` and `ToolGateway`, ensuring uniform security and auditing.

---

## 4. Vendor-Neutral Contracts

All agent structures are defined in [`contracts/agents/agent_execution_contracts.hpp`](file:///c:/Users/youri/OneDrive/Desktop/vani%20mark%202/contracts/agents/agent_execution_contracts.hpp):

### 4.1 `AgentResourceBudget`
- `max_steps`: 20
- `max_tool_calls`: 30
- `max_replans`: 3
- `max_retries_per_step`: 2
- `max_context_tokens`: 4,096
- `global_timeout_ms`: 300,000 ms (5 minutes)
- `default_step_timeout_ms`: 30,000 ms (30 seconds)

### 4.2 `AgentRequest`
- `request_id`: UUID/Turn correlation identifier.
- `goal`: User prompt / objective.
- `context`: Active window, recent turns, session data.
- `originating_session`: Session ID.
- `priority`: Execution priority (1–4).
- `deadline_ms`: Wall-clock deadline.
- `cancellation_token`: Cooperative cancellation token.
- `resource_budget`: Bound configurations.
- `permission_context`: Actor permission role (e.g., `user.agentic`).

### 4.3 `AgentStep` & `AgentPlan`
- `AgentStep`: `step_id`, `capability_id`, `tool_id`, `arguments` map, `dependencies` vector, `expected_postcondition`, `timeout_ms`, `retry_policy_max_attempts`, `risk_level`.
- `AgentPlan`: `plan_id`, `request_id`, `steps` list, `dependencies` edge list, `expected_outcomes`, `risk_level`, `estimated_cost`, `planner_provider`.

### 4.4 `AgentObservation`
- `step_id`: Step executed.
- `success`: Compound boolean (`tool_success && postcondition_verified`).
- `result_data`: Tool JSON response.
- `postcondition_verified`: Independent verification result.
- `evidence`: Empirical proof string.
- `error`: Error details if failed.
- `attempts_taken`: Number of attempts required.

---

## 5. Plan State Machine

Every plan transitions through explicit, observable states:
```text
  CREATED
     ↓
  PLANNING
     ↓
  VALIDATING  ──[Invalid / Injection]──> FAILED
     ↓
  WAITING_FOR_POLICY  ──[Deny]──> BLOCKED_POLICY
     ↓
  WAITING_FOR_CONFIRMATION  ──[User Rejects]──> BLOCKED_POLICY
     ↓
    READY
     ↓
  EXECUTING  ──[Cancel]──> CANCELLED
     ↓       ──[Timeout]──> TIMEOUT
  OBSERVING  ──[Prereq Failed]──> BLOCKED_DEPENDENCY
     ↓
  REPLANNING (Max 3) ──[Exhausted]──> FAILED
     ↓
  COMPLETED
```

Terminal states: `Completed`, `Failed`, `Cancelled`, `Timeout`, `BlockedPolicy`, `BlockedResource`, `BlockedDependency`.

---

## 6. Plan Validation & Adversarial Protection

The `PlanValidator` acts as the first defensive perimeter:
1. **Capability & Tool Registry**: Rejects any undeclared or unknown capabilities (`unauthorized.kernel_hack`).
2. **Cycle Detection**: Applies Kahn's topological sort algorithm to identify dependency cycles before execution begins.
3. **Command Injection Scrubbing**: Rejects arguments containing shell metacharacters (`;`, `&&`, `||`, `|`, `rm -rf`, `powershell -enc`, `format`, `sudo`).
4. **Adversarial Privilege Defense**: Rejects argument keys attempting to subvert policy (`override_policy`, `elevate_privilege`).
5. **Budget Adherence**: Rejects plans exceeding `max_steps`, unbounded retries (`retry_policy_max_attempts > 2`), or invalid timeouts.
6. **Mandatory Postconditions**: Mandates expected postconditions for state-mutating actions (`application.launch`, `application.close`, `filesystem.write`, `media.volume`).

---

## 7. Execution Scheduler & Postcondition Verification

The `ExecutionScheduler` coordinates step dispatching:
- **Dependency Ordering**: Computes DAG topological order. Steps only become eligible when all prerequisite dependencies have succeeded.
- **Concurrency Boundary**: Sequential execution (`MAX_PARALLEL_STEPS = 1`) by default.
- **Bounded Retries**: Automatically retries transient failures up to `max_retries_per_step` with exponential backoff. Strictly forbids retrying policy denials, permission errors, or invalid arguments.
- **Independent Postcondition Verification**:
  - `PostconditionVerifier` inspects the live operating system state independently of tool return values.
  - `application.launch` → verifies that the process exists in the live Windows Process Table.
  - `application.close` → verifies that the process is absent from the live process table.
  - `filesystem.write` → verifies that the file exists and is readable on disk.
  - If tool returns success but postcondition check fails → `VERIFICATION_FAILURE`.

---

## 8. Bounded Replanning & Ambiguity Resolution

The `Replanner` enables resilient recovery:
- **Replanning Limit**: Bounded to `MAX_REPLANS = 3`. Beyond 3 attempts, the task enters `PLAN_FAILED`.
- **Ambiguity Gate (`handle_ambiguity`)**:
  - If a step produces multiple valid candidates (e.g. two matching project directories or files), the agent does **not** guess arbitrarily.
  - The system returns `AMBIGUOUS_TARGET` and pauses for user clarification.

---

## 9. Task Memory & Context Management

The `ShortTermTaskMemory` manages active session memory:
- **Task-Scoped**: Observations, tool returns, and intermediate progress are retained only for the duration of the task.
- **Token Budget & Compaction**: Compacts older observations into summary representations when context approaches `max_context_tokens`.
- **Credential Scrubbing**: Regex filters automatically scrub passwords, bearer tokens, API keys, and GitHub personal access tokens (`ghp_...`) before storage.

---

## 10. Model Provider & Out-of-Process Worker Boundary

The `AgentModelProvider` decouples the agent core from external model vendors:
1. `DeterministicRulePlanner`: Fast, offline, deterministic rule-based planner for standard multi-step and workflow commands.
2. `LocalLLMPlanner`: Local Ollama / GGUF model adapter. Detects network/offline status and returns `AGENT_UNAVAILABLE_OFFLINE` when disconnected in offline mode.
3. `MockOrTestPlanner`: Programmable provider for unit, timeout, cycle, and adversarial test scenarios.
4. `ResourceGovernor`: Evaluates available system RAM and CPU load. If available RAM is < 500 MB, the task is safely rejected as `RESOURCE_CONSTRAINED`.

---

## 11. Observability & Audit Trail

### 11.1 Agent Telemetry Markers (A0–A13)
The agent runtime emits 14 dedicated telemetry markers without altering existing voice markers (`T0`–`T18`):
- `A0`: `agent_request`
- `A1`: `planning_start`
- `A2`: `planning_complete`
- `A3`: `validation_complete`
- `A4`: `policy_check`
- `A5`: `step_start`
- `A6`: `tool_request`
- `A7`: `tool_complete`
- `A8`: `verification`
- `A9`: `observation`
- `A10`: `replan`
- `A11`: `task_complete`
- `A12`: `task_failed`
- `A13`: `cancellation`

### 11.2 Audit Journal
`AgentAuditJournal` logs every invocation:
- `request_id`, `plan_id`, `step_id`, `capability`, `tool`, argument hashes/scrubbed values, policy decisions, execution results, verification evidence, retry counts, timestamps, and final outcomes.
