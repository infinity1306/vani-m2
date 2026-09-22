-- VANI Mark 2 Initial Schema Migration 001
-- SQLite schema for core control plane tables

CREATE TABLE IF NOT EXISTS schema_migrations (
    version INTEGER PRIMARY KEY,
    applied_at_ms INTEGER NOT NULL,
    description TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS tasks (
    task_id TEXT PRIMARY KEY,
    session_id TEXT NOT NULL,
    parent_task_id TEXT,
    title TEXT NOT NULL,
    description TEXT,
    category TEXT NOT NULL,
    priority INTEGER NOT NULL,
    state INTEGER NOT NULL,
    progress_percent INTEGER NOT NULL DEFAULT 0,
    created_at_ms INTEGER NOT NULL,
    updated_at_ms INTEGER NOT NULL,
    completed_at_ms INTEGER DEFAULT 0,
    result_summary TEXT,
    error_json TEXT,
    checkpoint_id TEXT
);

CREATE TABLE IF NOT EXISTS sessions (
    session_id TEXT PRIMARY KEY,
    user_id TEXT NOT NULL,
    device_id TEXT NOT NULL,
    started_at_ms INTEGER NOT NULL,
    last_activity_ms INTEGER NOT NULL,
    is_active INTEGER NOT NULL DEFAULT 1,
    context_reference TEXT
);

CREATE TABLE IF NOT EXISTS permissions (
    id TEXT PRIMARY KEY,
    actor_id TEXT NOT NULL,
    capability_permission TEXT NOT NULL,
    scope TEXT NOT NULL,
    granted_at_ms INTEGER NOT NULL,
    expires_at_ms INTEGER DEFAULT 0,
    reason TEXT
);

CREATE TABLE IF NOT EXISTS audit_records (
    record_id TEXT PRIMARY KEY,
    timestamp_ms INTEGER NOT NULL,
    actor_id TEXT NOT NULL,
    session_id TEXT,
    task_id TEXT,
    capability_id TEXT,
    action TEXT NOT NULL,
    decision TEXT NOT NULL,
    result TEXT NOT NULL,
    details_json TEXT
);

CREATE TABLE IF NOT EXISTS scheduled_jobs (
    job_id TEXT PRIMARY KEY,
    task_spec_json TEXT NOT NULL,
    schedule_type INTEGER NOT NULL,
    target_time_ms INTEGER NOT NULL,
    interval_ms INTEGER DEFAULT 0,
    is_recurring INTEGER DEFAULT 0,
    is_enabled INTEGER DEFAULT 1,
    created_at_ms INTEGER NOT NULL
);

INSERT OR IGNORE INTO schema_migrations (version, applied_at_ms, description)
VALUES (1, strftime('%s','now') * 1000, 'Initial VANI Mark 2 schema');
