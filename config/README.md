# config

## Responsibility

Runtime configuration and short-term local model files for Phase 5.5 testing.

## Boundary

`models/` contains local model definitions used by `ModelRepository`. Long term, model storage should move behind the repository boundary and may be backed by a database, object storage, or an external model service.

## Current State

- `models/M1.yaml` is the preferred local model definition for `model = M1`.
- `model.txt` remains as a compatibility fallback for older Phase 5.5 runs.

## Design Rules

- Console commands should pass `model` and `model_version`, not full YAML text.
- `ModelParser` parses model text only; it does not decide where model files live.
- Avoid growing this directory into a permanent large model store.

## Change Log

- 2026-05-15: Added `models/M1.yaml` and documented the ModelRepository boundary.
- 2026-05-14: Added M1 model definition for DataPoint auto initialization.
