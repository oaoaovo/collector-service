# HttpServer

## Responsibility

Local HTTP query and task-control API.

## Boundary

Phase 7 exposes local-only JSON APIs for service health, devices, command dispatch, collection task controls, runtime status, and latest Resources queries.

## Current State

Implemented in `include/HttpServer.h` and `src/HttpServer.cpp` using Boost.Beast.

## Allowed Changes

- Maintain HTTP route definitions and JSON response behavior.

## Forbidden Changes

- Do not add frontend pages, file upload, or user authentication in this phase.

## Design Rules

HTTP APIs delegate device mutations and task controls to `CommandProcessor` and use `DataTask`/`SQLiteManager` only for read APIs.

## Acceptance

`--http` starts a local server on `127.0.0.1:8080` by default. All routes return JSON.

## Change Log

- 2026-05-19: Implemented Phase 7 HttpServer.
- 2026-05-14: Added Phase 1 inactive module README.
