# DataTask

## Responsibility

Multi-device collection scheduling, response processing, status tracking, and cleanup triggering.

## Boundary

Active through Phase 8. This module may run independent background collection loops for online devices and trigger non-fatal Resources cleanup after successful collection.

## Current State

`include/DataTask.h` and `src/DataTask.cpp` implement start/stop entry points, DataPoint auto-initialization, per-device worker threads, DeviceStatus tracking, and periodic cleanup triggering.

## Allowed Changes

- DataTask implementation
- DataTask documentation

## Forbidden Changes

- Do not implement HTTP APIs.
- Do not add complex retry behavior.

## Design Rules

DataTask validates the device, initializes DataPoint from Device.model when needed, sends query through DriverClient, parses MockDriverServer response, inserts Resources, updates DeviceStatus, and triggers cleanup through SQLiteManager. Cleanup failure must be logged but must not change the successful collection result.

## Acceptance

`start_device_collect` starts one named worker. Successful collection clears `failCount` and `error`; failed collection increments only that device's `failCount`. Cleanup keeps Resources bounded without stopping the worker on cleanup errors.

## Change Log

- 2026-05-19: Added Phase 8 cleanup triggering and stage-specific error reporting.
- 2026-05-14: Added manual named-device collection and model-based DataPoint initialization.
- 2026-05-14: Activated Phase 5 DataTask implementation.
- 2026-05-14: Added Phase 1 inactive module README.
