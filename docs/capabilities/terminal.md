# Terminal Capability Specification

## 1. Overview
The Terminal Capability executes commands in a strictly controlled environment with policy classification, execution modes, structured output limits, working directory resolution, and cancellation support.

## 2. Capabilities
- `terminal.execute`: Executes a shell command and returns structured results (`exit_code`, `stdout`, `stderr`, `duration_ms`, `timed_out`, `cancelled`, `output_truncated`).

## 3. Command Risk Classification
Commands are classified into 5 risk levels:
1. **SAFE**: `ls`, `dir`, `pwd`, `git status`, `date`, `time`.
2. **LOW**: `git diff`, `git log`, `echo`, `cat`, `grep`, `whoami`.
3. **MEDIUM**: `npm install`, `npm test`, `cargo build`, `pytest`, `cmake`.
4. **HIGH**: `rm`, `del`, `chmod`, `kill`, `taskkill`, `git push --force`.
5. **CRITICAL**: `format`, `mkfs`, `shutdown`, `rm -rf /`, `curl ... | bash`.

## 4. Execution Modes
- `Normal`: Runs SAFE, LOW, MEDIUM, HIGH commands (CRITICAL blocked).
- `Restricted`: Runs SAFE, LOW, MEDIUM commands only.
- `Sandboxed`: Runs SAFE and LOW read-only commands only.

## 5. Working Directory Resolution Hierarchy
Priority:
1. Explicit CWD in request
2. Task CWD
3. Active Project CWD
4. Session CWD
5. Default Workspace (`/workspace`)
