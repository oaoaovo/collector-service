# collector-service

## Responsibility

This project is a C++17 industrial device data collection service. The current build adds command-driven device management, model repository loading, model-based DataPoint initialization, multi-device background collection, and HTTP query/control APIs.

## Boundary

The collector service owns a SQLite-backed device store, can create/update/delete devices through command JSON, can initialize DataPoint rows from a model definition, can run a mock driver WebSocket server, can run independent background collection tasks for multiple online devices, and can expose those operations through a local HTTP server.

## Current State

Active scope: Phase 8 stability hardening, HTTP server mode, command-driven device management, model-based DataPoint initialization, multi-device collection threads, per-device runtime status, and latest resource queries.

Implemented:

- CMake project
- `Logger` wrapper
- core model structs in `Models.h`
- startup entry point in `main.cpp`
- `SQLiteManager` opens `data/collector.sqlite`
- `scripts/init.sql` creates `Device`, `DataPoint`, and `Resources`
- `MockDriverServer` listens on `ws://127.0.0.1:9001` for local console and command-file verification
- `DriverClient` connects to a configured device driver address, sends query JSON text, and receives response JSON text
- `CommandProcessor` handles `create_device`, `delete_device`, `update_device`, `start_device_collect`, `stop_device_collect`, `start_all`, `stop_all`, and `get_status`
- `ModelRepository` loads model definitions by `model` and `model_version`
- `ModelParser` maps model DataItem entries into DataPoint rows
- `create_device` initializes DataPoint rows from the selected model immediately
- `update_device` refreshes DataPoint rows when model or model version changes
- `Device.Status` marks whether a device is online; only online devices can start collection
- `start_device_collect` starts one named background collection task that uses the device's own `IpAddr` and `Port` without stopping other devices
- `stop_device_collect` requests the named task to stop and the sleep loop checks that request every 100 ms
- `start_all` starts independent collection workers for all online devices
- `stop_all` stops all active collection workers
- `get_status` returns the latest tracked status for each device
- `HttpServer` exposes local JSON APIs for health, device listing, command dispatch, task start/stop, status, and latest Resources queries
- `SQLiteManager` can clean Resources history per device while preserving each device's newest rows
- `DataTask` triggers per-device Resources cleanup after successful collection without failing the collection loop if cleanup fails
- collection errors update only the affected device status with `failCount` and a stage-specific error message

## Resources Cleanup

Phase 8 keeps Resources growth bounded without changing the table schema. The default retention policy is:

```text
maxRowsPerDevice = 5000
cleanupIntervalMs = 60000
```

Each device cleanup keeps the newest rows for that `DeviceName`, ordered by `SvrTime DESC, Id DESC`. Cleanup only deletes from `Resources`; it never deletes `Device` or `DataPoint` rows. `DataTask` attempts cleanup after successful collection at the configured interval, and cleanup failure is logged as a warning without incrementing the device `failCount`.

## Model Files

Short-term Phase 6 tests use local model files:

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

HTTP mode:

```bash
./build/collector-service --http
```

The default HTTP endpoint is `http://127.0.0.1:8080`. Pass a port to override it:

```bash
./build/collector-service --serve-http 18080
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

Start all online devices:

```json
{"type":"json","uid":"admin","pid":"123456","cmd":{"category":"start_all","params":{}},"mid":"start-all"}
```

Query collection status:

```json
{"type":"json","uid":"admin","pid":"123456","cmd":{"category":"get_status","params":{}},"mid":"status"}
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

In command-file mode the mock driver starts only for `start_device_collect` and `start_all` commands. A start command keeps the collection process running until Enter is pressed, then active tasks and the mock server stop. The collection path never falls back to `127.0.0.1:9001`; if a device says `IpAddr=x` and `Port=y`, `DriverClient` connects only to `ws://x:y`.

## HTTP API

```bash
curl http://127.0.0.1:8080/health
curl http://127.0.0.1:8080/devices
curl http://127.0.0.1:8080/status
curl "http://127.0.0.1:8080/status?device=devA"
curl "http://127.0.0.1:8080/latest?device=devA"
curl -X POST http://127.0.0.1:8080/tasks/start
curl -X POST "http://127.0.0.1:8080/tasks/start?device=devA"
curl -X POST http://127.0.0.1:8080/tasks/stop
curl -X POST "http://127.0.0.1:8080/tasks/stop?device=devA"
```

Raw command JSON can also be sent through `POST /commands`; the request body is the same one-line command JSON used by console mode.

## Change Log

- 2026-05-19: Phase 7 HTTP server added with local JSON APIs for commands, task control, status, devices, health, and latest Resources.
- 2026-05-19: Phase 8 Resources cleanup and collection stability hardening added.
- 2026-05-18: Added Phase 6 multi-device DataTask scheduling with per-device worker threads, stop flags, status tracking, and `start_all`/`stop_all`/`get_status` commands.
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
