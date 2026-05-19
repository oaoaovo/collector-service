#pragma once

#include "Models.h"

#include <memory>
#include <string>
#include <vector>

namespace db {
class DataPointRepository;
class DeviceRepository;
class ResourceRepository;
class ResourceRetentionService;
class SQLiteConnection;
} // namespace db

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
    void cleanupResources(int maxRowsPerDevice);
    void cleanupResourcesForDevice(const std::string& deviceName, int maxRows);

private:
    void initializeSchema(const std::string& initSqlPath);

    std::unique_ptr<db::SQLiteConnection> connection_;
    std::unique_ptr<db::DeviceRepository> deviceRepository_;
    std::unique_ptr<db::DataPointRepository> dataPointRepository_;
    std::unique_ptr<db::ResourceRepository> resourceRepository_;
    std::unique_ptr<db::ResourceRetentionService> resourceRetentionService_;
};
