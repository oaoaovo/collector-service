# MockDriverServer

## Responsibility

WebSocket server that simulates an industrial driver service.

## Boundary

Active in Phase 3. This module may listen for WebSocket query JSON and return mock driver response JSON.

## Current State

`include/MockDriverServer.h` and `src/MockDriverServer.cpp` implement the Phase 3 WebSocket server.

## Allowed Changes

- MockDriverServer implementation
- MockDriverServer documentation

## Forbidden Changes

- Do not implement DriverClient.
- Do not implement DataTask.
- Do not write Resources.
- Do not implement HTTP APIs.

## Design Rules

The mock driver must use real WebSocket communication via Boost.Beast. It returns `machineConnected=true` and a `data` object keyed by query path.

## Acceptance

Connect to `ws://127.0.0.1:9001`, send query JSON, and receive response JSON containing the same `cid` and requested paths.

## Change Log

- 2026-05-14: Activated Phase 3 MockDriverServer implementation.
- 2026-05-14: Added Phase 1 inactive module README.
