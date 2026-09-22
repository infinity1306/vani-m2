# Application Management Specification

## 1. Overview
The Application Capability controls installed applications using deterministic alias resolution and platform adapters.

## 2. Capabilities
- `system.application.open`: Launches an authorized application.
- `system.application.close`: Gracefully closes an application.
- `system.application.focus`: Brings application window to the foreground.
- `system.application.restart`: Restarts an application.
- `system.application.list`: Lists installed or running applications.

## 3. Deterministic Application Registry
Instead of relying on ambiguous model hallucinations, `ApplicationRegistry` indexes canonical app identifiers and aliases (e.g. `VS Code`, `Visual Studio Code`, `Code` -> `com.microsoft.vscode` / `code.exe`).
