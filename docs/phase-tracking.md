# Phase Tracking

## Phase 8 - Resources Cleanup And Stability

Status: implemented.

Implemented capabilities:

- `SQLiteManager::cleanupResources(maxRowsPerDevice)` trims Resources globally by `DeviceName`.
- `SQLiteManager::cleanupResourcesForDevice(deviceName, maxRows)` trims one device's Resources only.
- Cleanup keeps newest records by `SvrTime DESC, Id DESC`.
- Cleanup uses SQLite transactions and does not touch `Device` or `DataPoint`.
- `DataTask` triggers per-device cleanup after successful collection with defaults `maxRowsPerDevice = 5000` and `cleanupIntervalMs = 60000`.
- Cleanup failure is logged as warning and does not increment collection `failCount`.
- Collection failures update only the affected device's `DeviceStatus`.
- Consecutive failures are still capped at 5 for the affected worker.
- Logs and status errors include the failing stage, such as `connect`, `sendQuery`, `processResponse`, or `cleanupResources`.

Manual verification should cover multi-device cleanup isolation, mock driver outage, invalid response data, SQLite write failure, and recovery after successful collection.
