# tests

## Responsibility

Tests for implemented phases.

## Boundary

No automated tests are required in Phase 1.

## Current State

Build-and-run verification is documented in the root README. Phase 8 adds manual stability checks for Resources cleanup and per-device failure isolation.

## Allowed Changes

- README maintenance.

## Forbidden Changes

- Do not add tests for unimplemented modules.

## Design Rules

Tests should follow implemented behavior phase by phase.

## Acceptance

Manual build and startup verification succeeds. For Phase 8, also verify:

- Starting collection for one online device writes Resources and reports `failCount = 0`.
- Stopping the mock driver or pointing one device at an unused port increments only that device's `failCount`.
- Five consecutive failures stop or mark only the affected device collection thread.
- Restarting a healthy mock driver allows successful collection to clear `failCount` and `error`.
- Resources cleanup keeps at most `maxRowsPerDevice` newest rows per `DeviceName` and does not remove another device's rows.
- Cleanup failure is logged but does not stop the collection loop.

## Change Log

- 2026-05-19: Added Phase 8 manual stability verification notes.
- 2026-05-14: Added Phase 1 placeholder README.
