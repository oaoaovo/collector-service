# src

## Responsibility

Source files for executable behavior. This layer wires startup modes, command dispatch, HTTP routing, multi-device collection, driver communication, model parsing, SQLite persistence, and shared utilities.

## Boundary

Top-level source files contain service facades and integration entry points:

- `main.cpp`: startup mode selection, database initialization, mock driver lifetime, command-file/console/http modes, and shutdown.
- `SQLiteManager.cpp`: database facade that delegates to connection, repository, transaction, and retention helpers.
- `DataTask.cpp`: collection facade that delegates to task helpers and owns high-level start/stop/status behavior.
- `CommandProcessor.cpp`: command facade that delegates parsing and serialization.
- `HttpServer.cpp`: HTTP listener/session facade that delegates routing.
- `DriverClient.cpp`, `MockDriverServer.cpp`, `ModelParser.cpp`, and `ModelRepository.cpp`: driver and model integration implementations.

Subdirectories contain the extracted implementation details:

- `util/`: `Logger`, JSON escaping/quoting helpers, and time helpers.
- `command/`: command field extraction, command parameter parsing, and command result/status JSON serialization.
- `db/`: SQLite RAII wrappers, Device/DataPoint/Resources repositories, and Resources retention cleanup.
- `task/`: query construction, response parsing, device status state, and collection worker behavior.
- `http/`: HTTP response construction, route handlers, HTTP JSON serializers, and HTTP-to-command adaptation.

## Current State

The current executable supports:

- database initialization from `scripts/init.sql`;
- command-file and long-running console modes;
- local HTTP server mode through `--http` or `--serve-http <port>`;
- device CRUD through command JSON;
- model-based DataPoint initialization;
- one worker per active online device;
- start/stop for one device or all online devices;
- per-device runtime status and fail counts;
- latest Resources queries through HTTP;
- Resources cleanup that preserves each device's newest rows.

## Startup Modes

```text
collector-service --console
collector-service --cmd-file <path>
collector-service --http
collector-service --serve-http <port>
```

Command-file mode starts the mock driver only for collection-start commands and keeps the process alive until Enter is pressed. HTTP mode starts the HTTP server and keeps the process alive until Enter is pressed.

## Design Rules

- Keep `main.cpp` focused on lifecycle wiring and mode selection.
- Keep facades thin; put reusable behavior in the matching subdirectory helper.
- Collection code must use the device's configured `IpAddr` and `Port`; it should not silently fall back to the local mock driver.
- Stop requests should be observed quickly by active collection workers.
- Cleanup failures should be logged without failing otherwise successful collection loops.

## Acceptance

From the repository root:

```bash
cmake --build build
./build/collector-service --console
./build/collector-service --http
```

Expected behavior includes command JSON device management, multi-device collection task control, status reporting, HTTP health/device/status/latest endpoints, and bounded Resources table growth.

## Change Log

- 2026-05-19: Phase 6 documentation refresh for the refactored source layout.
- 2026-05-19: Added HTTP server source split and Resources retention cleanup source split.
- 2026-05-19: Added command, db, task, http, and util helper source groups.
- 2026-05-14: Added CommandProcessor and ModelParser source files.
- 2026-05-14: Added Phase 5 DataTask source and single-device startup verification.
- 2026-05-14: Added Phase 4 DriverClient source and temporary startup verification.
- 2026-05-14: Added Phase 3 MockDriverServer source and startup wiring.
- 2026-05-14: Added Phase 2 SQLiteManager source and startup readback.
- 2026-05-14: Added Phase 1 source files.
