# Clipboard Capability Specification

## 1. Overview
The Clipboard Capability provides secure access to the system clipboard while enforcing strict privacy invariants.

## 2. Capabilities
- `clipboard.read`: Reads clipboard text/content. Flagged as sensitive.
- `clipboard.write`: Writes text/content to clipboard.
- `clipboard.clear`: Clears clipboard content.

## 3. Privacy & Zero-Log Policy
To protect passwords, API tokens, and private data:
- Clipboard contents are NEVER written to logs, console, or persistent audit records.
- Audit records store only sanitized summaries (`[CLIPBOARD_CONTENT: Type=Text, Length=45, Sensitive=true]`).
