# data

## Responsibility

Runtime SQLite data storage.

## Boundary

Phase 2 creates `collector.sqlite` at runtime.

## Current State

Contains generated runtime database files after the executable starts.

## Allowed Changes

- README maintenance
- Generated `collector.sqlite` at runtime

## Forbidden Changes

- Do not add non-database runtime artifacts.

## Design Rules

Generated database files should not be treated as source files.

## Acceptance

The executable creates `collector.sqlite` and initializes the schema.

## Change Log

- 2026-05-14: Phase 2 enables runtime SQLite database creation.
- 2026-05-14: Added Phase 1 placeholder README.
