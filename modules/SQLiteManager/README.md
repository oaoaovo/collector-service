# SQLiteManager

## Responsibility

Owner of SQLite connection, schema initialization, database access, and Resources cleanup.

## Boundary

Active through Phase 8. This module initializes SQLite, exposes Device/DataPoint/Resources access, and owns Resources cleanup SQL.

## Current State

`include/SQLiteManager.h` and `src/SQLiteManager.cpp` implement the database layer and Phase 8 per-device Resources retention.

## Allowed Changes

- SQLiteManager implementation
- `scripts/init.sql`
- SQLiteManager documentation

## Forbidden Changes

- Do not implement DataTask collection loops.
- Do not implement WebSocket communication.
- Do not implement HTTP APIs.
- Do not move Resources cleanup SQL into DataTask or HTTP code.

## Design Rules

SQL access must remain centralized in SQLiteManager. Resources cleanup uses transactions and preserves the newest rows per device by `SvrTime DESC, Id DESC`.

## Acceptance

Program startup creates `data/collector.sqlite` and creates `Device`, `DataPoint`, and `Resources`. Cleanup can trim Resources globally or for one device without deleting Device/DataPoint rows.

## Change Log

- 2026-05-19: Added Phase 8 Resources cleanup APIs.
- 2026-05-14: Activated Phase 2 SQLiteManager implementation.
- 2026-05-14: Added Phase 1 inactive module README.
