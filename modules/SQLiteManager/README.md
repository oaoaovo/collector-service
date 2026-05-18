# SQLiteManager

## Responsibility

Owner of SQLite connection, schema initialization, and database access.

## Boundary

Active in Phase 2. This module may initialize SQLite and expose basic Device/DataPoint/Resources access.

## Current State

`include/SQLiteManager.h` and `src/SQLiteManager.cpp` implement the Phase 2 database layer.

## Allowed Changes

- SQLiteManager implementation
- `scripts/init.sql`
- SQLiteManager documentation

## Forbidden Changes

- Do not implement DataTask collection loops.
- Do not implement WebSocket communication.
- Do not implement HTTP APIs.
- Do not add resource cleanup policies before Phase 9.

## Design Rules

SQL access must remain centralized in SQLiteManager. Business modules should not scatter raw SQL when they arrive in later phases.

## Acceptance

Program startup creates `data/collector.sqlite` and creates `Device`, `DataPoint`, and `Resources`. Devices are created explicitly through commands.

## Change Log

- 2026-05-14: Activated Phase 2 SQLiteManager implementation.
- 2026-05-14: Added Phase 1 inactive module README.
