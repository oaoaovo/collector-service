#pragma once

#include "Models.h"
#include "ModelRepository.h"

#include <atomic>
#include <map>
#include <memory>
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
    void runDeviceTask(Device device, std::shared_ptr<std::atomic_bool> stopFlag);
    void processResponse(const Device& device,
                         const std::vector<DataPoint>& points,
                         const std::string& responseText);
    void sleepUntilNextCycle(int sampleIntervalMs, const std::shared_ptr<std::atomic_bool>& stopFlag);
    void setLatestStatus(const DeviceStatus& status);

    SQLiteManager& sqliteManager_;
    std::map<std::string, DeviceStatus> statuses_;
    std::set<std::string> activeDevices_;
    std::map<std::string, std::thread> workerThreads_;
    std::map<std::string, std::shared_ptr<std::atomic_bool>> stopFlags_;
    ModelRepository modelRepository_;
    mutable std::mutex stateMutex_;
};
