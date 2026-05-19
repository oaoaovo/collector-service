# include

## Responsibility

Public and internal headers for the collector service. This directory describes the service contracts between startup, HTTP routing, command processing, task scheduling, database persistence, driver access, model parsing, and shared utilities.

## Boundary

Top-level headers keep compatibility with the earlier layout and expose the main service facades:

- `Models.h`: shared data structures used across database, command, HTTP, and task code.
- `SQLiteManager.h`: database facade used by command, HTTP, and collection code.
- `DataTask.h`: collection facade used by commands, HTTP task endpoints, and startup shutdown handling.
- `CommandProcessor.h`: command JSON dispatch facade.
- `HttpServer.h`: local HTTP server facade.
- `DriverClient.h` and `MockDriverServer.h`: driver integration boundaries.
- `ModelParser.h` and `ModelRepository.h`: model loading and DataPoint initialization boundaries.

Subdirectories hold narrower helper contracts:

- `util/`: logging, JSON text helpers, and time helpers.
- `command/`: command parsing and command response serialization.
- `db/`: SQLite RAII wrappers, repositories, and Resources retention service.
- `task/`: collection query building, response parsing, status storage, and worker helpers.
- `http/`: response helpers, JSON serializers, command adapter, and router.

## Current State

The header layer reflects the refactored Phase 8 shape:

- command parsing/serialization is split out of `CommandProcessor`;
- HTTP request routing/serialization is split out of `HttpServer`;
- collection query building, response parsing, worker behavior, and status state are split out of `DataTask`;
- SQLite handle management, statements, transactions, repositories, and retention cleanup are split out of `SQLiteManager`;
- utilities live under `include/util`;
- compatibility headers remain at the top level where existing source includes still use them.

## Design Rules

- Keep headers dependency-light and prefer forward declarations where practical.
- Put reusable helper contracts in the matching subdirectory instead of adding more private logic to facades.
- Keep facade headers stable for callers; place volatile implementation details in `src/*` helpers.
- Do not add database, HTTP, or command JSON formatting logic directly to model structs.

## Acceptance

The executable target should compile with all headers in this tree and support:

- console and command-file command processing;
- `--http` / `--serve-http` startup;
- multi-device collection task control;
- latest Resources queries;
- per-device Resources retention cleanup.

## Change Log

- 2026-05-19: Phase 6 documentation refresh for the refactored include layout.
- 2026-05-19: Added HTTP, task, db, command, and util helper header groups.
- 2026-05-15: Added ModelRepository header.
- 2026-05-14: Added CommandProcessor and ModelParser headers.
- 2026-05-14: Added Phase 5 DataTask header.
- 2026-05-14: Added Phase 4 DriverClient header.
- 2026-05-14: Added Phase 3 MockDriverServer header.
- 2026-05-14: Added Phase 2 SQLiteManager header.
- 2026-05-14: Added Phase 1 headers.
