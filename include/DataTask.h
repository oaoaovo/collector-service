#pragma once

#include "Models.h"
#include "ModelRepository.h"

#include <atomic>
#include <mutex>
#include <string>
#include <set>
#include <thread>
#include <vector>

class SQLiteManager;

class DataTask {
public:
    explicit DataTask(SQLiteManager& sqliteManager);
    DataTask(SQLiteManager& sqliteManager, ModelRepository modelRepository);
    ~DataTask();

    DataTask(const DataTask&) = delete;
    DataTask& operator=(const DataTask&) = delete;

    void startAll();
    void stopAll();
    CommandResult startDeviceCollect(const std::string& deviceName, const std::string& mid);
    CommandResult stopDeviceCollect(const std::string& deviceName, const std::string& mid);
    CommandResult initDataPointsByDevice(const std::string& deviceName, const std::string& mid);
    std::vector<DeviceStatus> getStatus() const;

private:
    void runDeviceTask(Device device);
    void processResponse(const Device& device,
                         const std::vector<DataPoint>& points,
                         const std::string& responseText);
    void sleepUntilNextCycle(int sampleIntervalMs);
    void setLatestStatus(const DeviceStatus& status);

    SQLiteManager& sqliteManager_;
    std::vector<DeviceStatus> statuses_;
    std::set<std::string> activeDevices_;
    ModelRepository modelRepository_;
    mutable std::mutex stateMutex_;
    std::thread workerThread_;
    std::atomic_bool stopRequested_{false};
};
