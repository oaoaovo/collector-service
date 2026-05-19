#include "SQLiteManager.h"

#include "db/DataPointRepository.h"
#include "db/DeviceRepository.h"
#include "db/ResourceRepository.h"
#include "db/ResourceRetentionService.h"
#include "db/SQLiteConnection.h"
#include "db/SQLiteTransaction.h"

#include <fstream>
#include <mutex>
#include <sstream>
#include <stdexcept>

namespace {

std::string readTextFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("failed to open SQL file: " + path);
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool isDuplicateColumnError(const std::string& message) {
    return message.find("duplicate column name") != std::string::npos;
}

} // namespace

SQLiteManager::SQLiteManager(const std::string& databasePath, const std::string& initSqlPath)
    : connection_(std::make_unique<db::SQLiteConnection>(databasePath)),
      deviceRepository_(std::make_unique<db::DeviceRepository>(*connection_)),
      dataPointRepository_(std::make_unique<db::DataPointRepository>(*connection_)),
      resourceRepository_(std::make_unique<db::ResourceRepository>(*connection_)),
      resourceRetentionService_(std::make_unique<db::ResourceRetentionService>(*connection_)) {
    initializeSchema(initSqlPath);
}

SQLiteManager::~SQLiteManager() = default;

bool SQLiteManager::deviceExists(const std::string& deviceName) const {
    std::lock_guard<std::recursive_mutex> lock(connection_->mutex());
    return deviceRepository_->exists(deviceName);
}

Device SQLiteManager::getDeviceByName(const std::string& deviceName) const {
    std::lock_guard<std::recursive_mutex> lock(connection_->mutex());
    return deviceRepository_->getByName(deviceName);
}

void SQLiteManager::createDevice(const DeviceUpsert& device) {
    std::lock_guard<std::recursive_mutex> lock(connection_->mutex());
    if (deviceRepository_->exists(device.name)) {
        throw std::runtime_error("device already exists: " + device.name);
    }
    deviceRepository_->create(device);
}

void SQLiteManager::updateDevice(const DeviceUpsert& device) {
    std::lock_guard<std::recursive_mutex> lock(connection_->mutex());
    const auto targetName = !device.oldName.empty() ? device.oldName : device.name;
    const auto existing = deviceRepository_->getByName(targetName);
    if (device.name != targetName && deviceRepository_->exists(device.name)) {
        throw std::runtime_error("device already exists: " + device.name);
    }

    deviceRepository_->update(device, targetName);
    if (device.name != targetName) {
        dataPointRepository_->renameDevice(targetName, device.name);
    }
    if (existing.model != device.model || existing.modelVersion != device.modelVersion) {
        dataPointRepository_->deleteByDeviceName(device.name);
    }
}

void SQLiteManager::deleteDevice(const std::string& deviceName) {
    std::lock_guard<std::recursive_mutex> lock(connection_->mutex());
    if (!deviceRepository_->exists(deviceName)) {
        throw std::runtime_error("device not found: " + deviceName);
    }

    db::SQLiteTransaction transaction(*connection_);
    dataPointRepository_->deleteByDeviceName(deviceName);
    deviceRepository_->remove(deviceName);
    transaction.commit();
}

bool SQLiteManager::isDeviceOnline(const std::string& deviceName) const {
    std::lock_guard<std::recursive_mutex> lock(connection_->mutex());
    return deviceRepository_->getByName(deviceName).status != 0;
}

void SQLiteManager::deleteDataPointsByDeviceName(const std::string& deviceName) {
    std::lock_guard<std::recursive_mutex> lock(connection_->mutex());
    dataPointRepository_->deleteByDeviceName(deviceName);
}

void SQLiteManager::replaceDataPointsForDevice(const Device& device, const std::vector<DataPoint>& points) {
    std::lock_guard<std::recursive_mutex> lock(connection_->mutex());
    db::SQLiteTransaction transaction(*connection_);
    dataPointRepository_->replaceForDevice(device, points);
    transaction.commit();
}

std::vector<Device> SQLiteManager::getDevices() const {
    std::lock_guard<std::recursive_mutex> lock(connection_->mutex());
    return deviceRepository_->getAll();
}

std::vector<DataPoint> SQLiteManager::getDataPointsByDeviceName(const std::string& deviceName) const {
    std::lock_guard<std::recursive_mutex> lock(connection_->mutex());
    return dataPointRepository_->getByDeviceName(deviceName);
}

void SQLiteManager::insertResources(const std::vector<ResourceRecord>& records) {
    std::lock_guard<std::recursive_mutex> lock(connection_->mutex());
    db::SQLiteTransaction transaction(*connection_);
    resourceRepository_->insert(records);
    transaction.commit();
}

std::vector<ResourceRecord> SQLiteManager::getLatestResources(const std::string& deviceName) const {
    std::lock_guard<std::recursive_mutex> lock(connection_->mutex());
    return resourceRepository_->getLatest(deviceName);
}

void SQLiteManager::cleanupResources(int maxRowsPerDevice) {
    std::lock_guard<std::recursive_mutex> lock(connection_->mutex());
    db::SQLiteTransaction transaction(*connection_);
    resourceRetentionService_->cleanupResources(maxRowsPerDevice);
    transaction.commit();
}

void SQLiteManager::cleanupResourcesForDevice(const std::string& deviceName, int maxRows) {
    std::lock_guard<std::recursive_mutex> lock(connection_->mutex());
    db::SQLiteTransaction transaction(*connection_);
    resourceRetentionService_->cleanupResourcesForDevice(deviceName, maxRows);
    transaction.commit();
}

void SQLiteManager::initializeSchema(const std::string& initSqlPath) {
    std::lock_guard<std::recursive_mutex> lock(connection_->mutex());
    connection_->execute(readTextFile(initSqlPath));
    try {
        connection_->execute("ALTER TABLE Device ADD COLUMN Status INTEGER NOT NULL DEFAULT 1;");
    } catch (const std::exception& ex) {
        if (!isDuplicateColumnError(ex.what())) {
            throw;
        }
    }
}
