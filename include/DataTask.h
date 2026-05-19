#pragma once

#include "Models.h"
#include "ModelRepository.h"
#include "task/DeviceStatusStore.h"

#include <atomic>
#include <memory>
#include <string>
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
    void sleepUntilNextCycle(int sampleIntervalMs, const std::shared_ptr<std::atomic_bool>& stopFlag);

    SQLiteManager& sqliteManager_;
    task::DeviceStatusStore statusStore_;
    ModelRepository modelRepository_;
};
