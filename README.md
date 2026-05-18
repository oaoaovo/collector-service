# collector-service

## Responsibility

This project is a C++17 industrial device data collection service. The current build adds command-driven device management, model repository loading, and model-based DataPoint initialization.

## Boundary

The collector service owns a SQLite-backed device store, can create/update/delete devices through command JSON, can initialize DataPoint rows from a model definition, can run a mock driver WebSocket server, and can run one named background collection task. HTTP query APIs and multi-device scheduling are intentionally not implemented in Phase 5.5.

## Current State

Active scope: Phase 5.5 console command mode, command-driven device management, model-based DataPoint initialization, and one named background collection task.

Implemented:

- CMake project
- `Logger` wrapper
- core model structs in `Models.h`
- startup entry point in `main.cpp`
- `SQLiteManager` opens `data/collector.sqlite`
- `scripts/init.sql` creates `Device`, `DataPoint`, and `Resources`
- `MockDriverServer` listens on `ws://127.0.0.1:9001` for local console and command-file verification
- `DriverClient` connects to a configured device driver address, sends query JSON text, and receives response JSON text
- `CommandProcessor` handles `create_device`, `delete_device`, `update_device`, `start_device_collect`, and `stop_device_collect`
- `ModelRepository` loads model definitions by `model` and `model_version`
- `ModelParser` maps model DataItem entries into DataPoint rows
- `create_device` initializes DataPoint rows from the selected model immediately
- `update_device` refreshes DataPoint rows when model or model version changes
- `Device.Status` marks whether a device is online; only online devices can start collection
- `start_device_collect` starts one named background collection task that uses the device's own `IpAddr` and `Port`
- `stop_device_collect` requests the running task to stop and the sleep loop checks that request every 100 ms

## Model Files

Short-term Phase 5.5 tests use local model files:

```text
config/models/M1.yaml
```

Commands should pass only short model metadata, not full YAML text:

```json
{"model":"M1","model_version":"1.0"}
```

`config/model.txt` remains as a compatibility fallback for `M1`.

## Acceptance

From this directory:

```bash
cmake -S . -B build
cmake --build build
./build/collector-service --console
```

Console commands are one-line JSON. Create an online device first:

```json
{"type":"json","uid":"admin","pid":"123456","cmd":{"category":"create_device","params":{"device_name":"devA","model":"M1","model_version":"1.0","addr":"127.0.0.1:9001","SampleInterval":1000,"status":1,"driver_path":"mock-driver"}},"mid":"create-devA"}
```

Then start periodic collection for that device:

```json
{"type":"json","uid":"admin","pid":"123456","cmd":{"category":"start_device_collect","params":{"device_name":"devA"}},"mid":"start-devA"}
```

Stop collection when needed:

```json
{"type":"json","uid":"admin","pid":"123456","cmd":{"category":"stop_device_collect","params":{"device_name":"devA"}},"mid":"stop-devA"}
```

Exit console mode:

```text
exit
```

Expected output includes:

```text
console mode started
DataPoint initialized for device: devA
DataTask collection succeeded for devA
Command response: {"code":200,"msg":"device collection started: devA","mid":"start-devA"}
Command response: {"code":200,"msg":"device collection stopped: devA","mid":"stop-devA"}
```

One-shot command-file mode remains available:

```bash
./build/collector-service --cmd-file build/start_devA.json
```

In command-file mode the mock driver starts only for `start_device_collect` commands. A start command keeps the collection process running until Enter is pressed, then the task and mock server stop. The collection path never falls back to `127.0.0.1:9001`; if a device says `IpAddr=x` and `Port=y`, `DriverClient` connects only to `ws://x:y`.

## Change Log

- 2026-05-18: Added `Device.Status`, online-only collection, single named periodic collection loop, fast stop checks, and removed local mock fallback from `DataTask`.
- 2026-05-15: Phase 5.5 console mode added; startup demo and DeviceA seed behavior removed.
- 2026-05-15: Added ModelRepository and moved model lookup out of ModelParser.
- 2026-05-15: Phase 5.5 command flow completed: device CRUD, model DataPoint initialization, and manual collection command verification.
- 2026-05-14: Added device create/update/delete commands, model-based DataPoint initialization, and manual device collection commands.
- 2026-05-14: Phase 5 DataTask single-device collection chain added.
- 2026-05-14: Phase 4 DriverClient WebSocket client added.
- 2026-05-14: Phase 3 MockDriverServer WebSocket server added.
- 2026-05-14: Phase 2 SQLiteManager, schema initialization, seed data, and startup readback added.
- 2026-05-14: Phase 1 skeleton created.
