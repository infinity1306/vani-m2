# Filesystem Capability Specification

## 1. Overview
The Filesystem Capability provides a secure, sandboxed abstraction for file operations with path traversal protection, sensitive path guarding, transactional snapshot rollback, and safe-trash deletion.

## 2. Capabilities
- `filesystem.read`: Reads content of a file within authorized scope.
- `filesystem.write`: Writes content to a file, taking pre-operation snapshots for transaction rollback.
- `filesystem.create`: Creates a directory or file.
- `filesystem.move`: Relocates a file, recording undo metadata.
- `filesystem.copy`: Copies a file within authorized scope.
- `filesystem.rename`: Renames a file.
- `filesystem.delete`: Moves file to safe trash by default, enabling reversible recovery.
- `filesystem.search`: Searches files by name pattern.
- `filesystem.watch`: Monitors file changes for authorized paths.

## 3. Path Security & Scopes
- **Path Traversal Prevention**: Strips and rejects `../`, `..\\`, and symlink escapes.
- **Sensitive Zones**: Prohibits agent access to OS root (`/etc/shadow`, `C:/Windows/System32`), credential directories (`.ssh`, `.aws`, `.kube`, `.gnupg`), and private key files (`id_rsa`, `id_ed25519`).
- **Scoped Permissions**: Grants can be scoped to specific project directories (`filesystem.read:/workspace/vani/`).

## 4. Reversibility & Rollback
- Multi-file operations can be bound to a `transaction_id`.
- Calling `rollback_transaction(tx_id)` reverses all file modifications, creations, renames, and deletions in reverse order.
