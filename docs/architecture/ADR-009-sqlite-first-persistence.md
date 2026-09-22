# ADR-009: Repository Pattern & SQLite-First Persistence

## Status
Accepted

## Context
Directly embedding raw SQL queries inside business logic tightly couples the task system, memory layer, and audit logger to a specific storage schema.

## Decision
All persistence is abstracted behind strongly typed Repository interfaces (`TaskRepository`, `MemoryRepository`, `DeviceRepository`, `AuditRepository`). SQLite (with WAL mode) serves as the primary local embedded implementation. Switching to an encrypted local database or multi-node sync store requires zero modifications to the task manager or memory services.

## Consequences
- **Positive**: Complete database portability and isolation.
