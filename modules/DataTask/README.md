# DataTask

## Responsibility

Manual single-device collection chain and response processor.

## Boundary

Active in Phase 5 and extended by the requested command retrofit. This module may run one manual collection pass for a named device.

## Current State

`include/DataTask.h` and `src/DataTask.cpp` implement manual start/stop entry points, DataPoint auto-initialization, and the single-device collection chain.

## Allowed Changes

- DataTask implementation
- DataTask documentation

## Forbidden Changes

- Do not add multi-device concurrent scheduling.
- Do not add background collection loops.
- Do not implement HTTP APIs.
- Do not add complex retry behavior.

## Design Rules

DataTask validates the device, initializes DataPoint from Device.model when needed, sends query through DriverClient, parses MockDriverServer response, inserts Resources, and updates a basic DeviceStatus vector.

## Acceptance

Manual `start_device_collect` initializes model points and inserts Resources for those paths after one collection pass.

## Change Log

- 2026-05-14: Added manual named-device collection and model-based DataPoint initialization.
- 2026-05-14: Activated Phase 5 DataTask implementation.
- 2026-05-14: Added Phase 1 inactive module README.
