# Change Log

## 2026-05-18

- Added `Device.Status` with SQLite migration support so devices can be marked online/offline.
- Changed `start_device_collect` to require an online device and start one named periodic background task.
- Added fast stop checks during sample-interval sleep so `stop_device_collect` can stop promptly.
- Removed the `127.0.0.1:9001` fallback from `DataTask`; collection now uses only the device's configured `IpAddr` and `Port`.
- Hardened `DriverClient` reconnect behavior by resetting socket state after failed connect, failed send, or close.

## 2026-05-15

- Added real `--console` mode for one-line command JSON input.
- Removed startup DeviceA demo and seed behavior.
- Added `ModelRepository` so model lookup is separate from model parsing.
- Added `config/models/M1.yaml` for short-term model-file based Phase 5.5 testing.
- Completed Phase 5.5 command flow.
- Fixed broken command/DataTask response strings that prevented compilation.
- `create_device` now initializes DataPoint rows from the selected model immediately.
- `update_device` refreshes DataPoint rows when model or model version changes.
- Verified create, update, and manual start collection command files against the rebuilt executable.

## 2026-05-14

- Added requested retrofit for Device command CRUD, model-based DataPoint initialization, and manual collection commands.
- Added `CommandProcessor` for `create_device`, `delete_device`, `update_device`, `start_device_collect`, and `stop_device_collect`.
- Added `ModelParser` and `config/model.txt` mapping M1 DataItem definitions into DataPoint rows.
- Implemented Phase 5 DataTask single-device collection chain.
- Added WebSocket response parsing into ResourceRecord objects.
- Wired `main.cpp` to run one DataTask pass and print latest Resources rows.
- Implemented Phase 4 DriverClient using Boost.Beast WebSocket client.
- Wired `main.cpp` to start MockDriverServer, send a temporary query through DriverClient, print the response, and exit.
- Implemented Phase 3 MockDriverServer using Boost.Beast WebSocket server.
- Wired `main.cpp` to start `MockDriverServer` on `127.0.0.1:9001`.
- Added mock JSON response generation for query paths.
- Implemented Phase 2 SQLiteManager.
- Added `scripts/init.sql` for `Device`, `DataPoint`, and `Resources`.
- Added startup database initialization, seed data insertion, and Device/DataPoint readback logging.
- Created the Phase 1 `collector-service` C++17 project skeleton.
- Added CMake build configuration.
- Added `Logger` initialization and logging methods.
- Added v2 design model structs in `Models.h`.
- Added `main.cpp` startup message: `collector service started`.
