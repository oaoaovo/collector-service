# src

## Responsibility

Source files for executable behavior.

## Boundary

The source layer contains logger initialization, SQLite initialization, command handling, model parsing, MockDriverServer startup, and one named periodic DataTask collection loop.

## Current State

`main.cpp` initializes `Logger`, opens `data/collector.sqlite`, applies `scripts/init.sql`, supports long-running `--console`, and can handle one-shot `--cmd` / `--cmd-file` commands.

## Allowed Changes

- `Logger.cpp`
- `main.cpp`
- `SQLiteManager.cpp`
- `MockDriverServer.cpp`
- `DriverClient.cpp`
- `DataTask.cpp`
- `ModelParser.cpp`
- `CommandProcessor.cpp`

## Forbidden Changes

- No HTTP startup
- No startup demo collection
- No multi-device collection loop

## Design Rules

Keep startup limited to database initialization, command handling, MockDriverServer, and one named DataTask worker until later phases authorize scheduler behavior.

## Acceptance

The executable supports command JSON files and verifies manual collection by initializing DataPoint from the model, writing Resources, and exiting.

## Change Log

- 2026-05-14: Added CommandProcessor and ModelParser source files.
- 2026-05-14: Added Phase 5 DataTask source and single-device startup verification.
- 2026-05-14: Added Phase 4 DriverClient source and temporary startup verification.
- 2026-05-14: Added Phase 3 MockDriverServer source and startup wiring.
- 2026-05-14: Added Phase 2 SQLiteManager source and startup readback.
- 2026-05-14: Added Phase 1 source files.
