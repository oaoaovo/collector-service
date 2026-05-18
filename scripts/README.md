# scripts

## Responsibility

Database and utility scripts.

## Boundary

Phase 2 uses `init.sql` to create the SQLite schema.

## Current State

Contains `init.sql`.

## Allowed Changes

- `init.sql`
- README maintenance

## Forbidden Changes

- Do not add scripts for later phases before they are active.

## Design Rules

SQL schema changes must remain centralized in this directory and executed through SQLiteManager.

## Acceptance

`init.sql` creates `Device`, `DataPoint`, and `Resources`.

## Change Log

- 2026-05-14: Added Phase 2 SQLite schema.
- 2026-05-14: Added Phase 1 placeholder README.
