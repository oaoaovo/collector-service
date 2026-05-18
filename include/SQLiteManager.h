#pragma once

#include "Models.h"

#include <sqlite3.h>

#include <mutex>
#include <string>
#include <vector>

class SQLiteManager {
public:
    explicit SQLiteManager(const std::string& databasePath = "data/collector.sqlite",
                           const std::string& initSqlPath = "scripts/init.sql");
    ~SQLiteManager();

    SQLiteManager(const SQLiteManager&) = delete;
    SQLiteManager& operator=(const SQLiteManager&) = delete;

    bool deviceExists(const std::string& deviceName) const;
    Device getDeviceByName(const std::string& deviceName) const;
    void createDevice(const DeviceUpsert& device);
    void updateDevice(const DeviceUpsert& device);
    void deleteDevice(const std::string& deviceName);
    bool isDeviceOnline(const std::string& deviceName) const;
    void deleteDataPointsByDeviceName(const std::string& deviceName);
    void replaceDataPointsForDevice(const Device& device, const std::vector<DataPoint>& points);
    std::vector<Device> getDevices() const;
    std::vector<DataPoint> getDataPointsByDeviceName(const std::string& deviceName) const;
    void insertResources(const std::vector<ResourceRecord>& records);
    std::vector<ResourceRecord> getLatestResources(const std::string& deviceName) const;

private:
    void openDatabase();
    void initializeSchema(const std::string& initSqlPath);
    void execute(const std::string& sql) const;
    bool hasRows(const std::string& sql) const;

    std::string databasePath_;
    sqlite3* db_ = nullptr;
    mutable std::recursive_mutex dbMutex_;
};
