# DriverClient

## Responsibility

WebSocket client for communicating with the driver service.

## Boundary

Active in Phase 4. This module may connect to a driver WebSocket endpoint, send query JSON text, receive response JSON text, and close the connection.

## Current State

`include/DriverClient.h` and `src/DriverClient.cpp` implement the Phase 4 WebSocket client.

## Allowed Changes

- DriverClient implementation
- DriverClient documentation

## Forbidden Changes

- Do not implement DataTask.
- Do not write Resources.
- Do not implement HTTP APIs.
- Do not add reconnect strategies before later stability phases.

## Design Rules

Driver communication must use real WebSocket transport via Boost.Beast.

## Acceptance

Program startup connects to `ws://127.0.0.1:9001`, sends query JSON, and prints a response containing `machineConnected=true` and `/Position/temperature`.

## Change Log

- 2026-05-14: Activated Phase 4 DriverClient implementation.
- 2026-05-14: Added Phase 1 inactive module README.
