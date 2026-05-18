# include

## Responsibility

Public headers for the collector service.

## Boundary

The include layer exposes `Logger.h`, `Models.h`, `SQLiteManager.h`, `MockDriverServer.h`, `DriverClient.h`, `DataTask.h`, `ModelParser.h`, `ModelRepository.h`, and `CommandProcessor.h`.

## Current State

`Models.h` contains the core data structures. `SQLiteManager.h` declares Device CRUD, DataPoint replacement, and resource access. `ModelRepository.h` declares model lookup. `ModelParser.h` declares model-to-DataPoint parsing. `CommandProcessor.h` declares command JSON dispatch.

## Allowed Changes

- Phase 5 DataTask declaration
- Device command, model repository, and model parser declarations
- Existing DriverClient declaration
- Existing MockDriverServer declaration
- Existing SQLiteManager declaration
- Existing model and logger declarations

## Forbidden Changes

- No HTTP server header
- No scheduler header for Phase 6 behavior

## Design Rules

Headers should remain small and dependency-light.

## Acceptance

The project compiles with SQLiteManager, MockDriverServer, DriverClient, DataTask, ModelRepository, ModelParser, and CommandProcessor included in the executable target.

## Change Log

- 2026-05-15: Added ModelRepository header.
- 2026-05-14: Added CommandProcessor and ModelParser headers.
- 2026-05-14: Added Phase 5 DataTask header.
- 2026-05-14: Added Phase 4 DriverClient header.
- 2026-05-14: Added Phase 3 MockDriverServer header.
- 2026-05-14: Added Phase 2 SQLiteManager header.
- 2026-05-14: Added Phase 1 headers.
