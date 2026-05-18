#include "SQLiteManager.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

struct Statement {
    Statement(sqlite3* db, const std::string& sql) : dbHandle(db) {
        if (sqlite3_prepare_v2(dbHandle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(dbHandle));
        }
    }

    ~Statement() {
        if (stmt != nullptr) {
            sqlite3_finalize(stmt);
        }
    }

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    sqlite3* dbHandle = nullptr;
    sqlite3_stmt* stmt = nullptr;
};

std::string readTextFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("failed to open SQL file: " + path);
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void bindText(sqlite3_stmt* stmt, int index, const std::string& value) {
    if (sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        throw std::runtime_error("failed to bind text parameter");
    }
}

void stepDone(sqlite3* db, sqlite3_stmt* stmt) {
    const int result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
}

std::string columnText(sqlite3_stmt* stmt, int index) {
    const auto* text = sqlite3_column_text(stmt, index);
    return text != nullptr ? reinterpret_cast<const char*>(text) : "";
}

bool isDuplicateColumnError(const std::string& message) {
    return message.find("duplicate column name") != std::string::npos;
}

} // namespace

SQLiteManager::SQLiteManager(const std::string& databasePath, const std::string& initSqlPath)
    : databasePath_(databasePath) {
    openDatabase();
    initializeSchema(initSqlPath);
}

SQLiteManager::~SQLiteManager() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool SQLiteManager::deviceExists(const std::string& deviceName) const {
    Statement stmt(db_, "SELECT 1 FROM Device WHERE Name = ? LIMIT 1;");
    bindText(stmt.stmt, 1, deviceName);
    return sqlite3_step(stmt.stmt) == SQLITE_ROW;
}

Device SQLiteManager::getDeviceByName(const std::string& deviceName) const {
    Statement stmt(db_,
                   "SELECT Id, Name, IpAddr, Port, SampleInterval, Status, Model, ModelVersion, DriverPath "
                   "FROM Device WHERE Name = ? LIMIT 1;");
    bindText(stmt.stmt, 1, deviceName);

    if (sqlite3_step(stmt.stmt) != SQLITE_ROW) {
        throw std::runtime_error("device not found: " + deviceName);
    }

    Device device;
    device.id = sqlite3_column_int(stmt.stmt, 0);
    device.name = columnText(stmt.stmt, 1);
    device.ip = columnText(stmt.stmt, 2);
    device.port = sqlite3_column_int(stmt.stmt, 3);
    device.sampleIntervalMs = sqlite3_column_int(stmt.stmt, 4);
    device.status = sqlite3_column_int(stmt.stmt, 5);
    device.model = columnText(stmt.stmt, 6);
    device.modelVersion = columnText(stmt.stmt, 7);
    device.driverPath = columnText(stmt.stmt, 8);
    return device;
}

void SQLiteManager::createDevice(const DeviceUpsert& device) {
    if (deviceExists(device.name)) {
        throw std::runtime_error("device already exists: " + device.name);
    }

    Statement stmt(db_,
                   "INSERT INTO Device "
                   "(Name, IpAddr, Port, SampleInterval, Status, Model, ModelVersion, DriverPath) "
                   "VALUES (?, ?, ?, ?, ?, ?, ?, ?);");
    bindText(stmt.stmt, 1, device.name);
    bindText(stmt.stmt, 2, device.ip);
    sqlite3_bind_int(stmt.stmt, 3, device.port);
    sqlite3_bind_int(stmt.stmt, 4, device.sampleIntervalMs);
    sqlite3_bind_int(stmt.stmt, 5, device.status);
    bindText(stmt.stmt, 6, device.model);
    bindText(stmt.stmt, 7, device.modelVersion);
    bindText(stmt.stmt, 8, device.driverPath);
    stepDone(db_, stmt.stmt);
}

void SQLiteManager::updateDevice(const DeviceUpsert& device) {
    const auto targetName = !device.oldName.empty() ? device.oldName : device.name;
    const auto existing = getDeviceByName(targetName);
    if (device.name != targetName && deviceExists(device.name)) {
        throw std::runtime_error("device already exists: " + device.name);
    }

    Statement stmt(db_,
                   "UPDATE Device SET Name = ?, IpAddr = ?, Port = ?, SampleInterval = ?, "
                   "Status = ?, Model = ?, ModelVersion = ?, DriverPath = ? WHERE Name = ?;");
    bindText(stmt.stmt, 1, device.name);
    bindText(stmt.stmt, 2, device.ip);
    sqlite3_bind_int(stmt.stmt, 3, device.port);
    sqlite3_bind_int(stmt.stmt, 4, device.sampleIntervalMs);
    sqlite3_bind_int(stmt.stmt, 5, device.status);
    bindText(stmt.stmt, 6, device.model);
    bindText(stmt.stmt, 7, device.modelVersion);
    bindText(stmt.stmt, 8, device.driverPath);
    bindText(stmt.stmt, 9, targetName);
    stepDone(db_, stmt.stmt);

    if (device.name != targetName) {
        Statement renamePoints(db_, "UPDATE DataPoint SET DName = ? WHERE DName = ?;");
        bindText(renamePoints.stmt, 1, device.name);
        bindText(renamePoints.stmt, 2, targetName);
        stepDone(db_, renamePoints.stmt);
    }

    if (existing.model != device.model || existing.modelVersion != device.modelVersion) {
        deleteDataPointsByDeviceName(device.name);
    }
}

void SQLiteManager::deleteDevice(const std::string& deviceName) {
    if (!deviceExists(deviceName)) {
        throw std::runtime_error("device not found: " + deviceName);
    }

    execute("BEGIN TRANSACTION;");
    try {
        {
            Statement stmt(db_, "DELETE FROM DataPoint WHERE DName = ?;");
            bindText(stmt.stmt, 1, deviceName);
            stepDone(db_, stmt.stmt);
        }
        {
            Statement stmt(db_, "DELETE FROM Device WHERE Name = ?;");
            bindText(stmt.stmt, 1, deviceName);
            stepDone(db_, stmt.stmt);
        }
        execute("COMMIT;");
    } catch (...) {
        execute("ROLLBACK;");
        throw;
    }
}

bool SQLiteManager::isDeviceOnline(const std::string& deviceName) const {
    return getDeviceByName(deviceName).status != 0;
}

void SQLiteManager::deleteDataPointsByDeviceName(const std::string& deviceName) {
    Statement stmt(db_, "DELETE FROM DataPoint WHERE DName = ?;");
    bindText(stmt.stmt, 1, deviceName);
    stepDone(db_, stmt.stmt);
}

void SQLiteManager::replaceDataPointsForDevice(const Device& device, const std::vector<DataPoint>& points) {
    execute("BEGIN TRANSACTION;");
    try {
        {
            Statement stmt(db_, "DELETE FROM DataPoint WHERE DName = ?;");
            bindText(stmt.stmt, 1, device.name);
            stepDone(db_, stmt.stmt);
        }

        Statement stmt(db_,
                       "INSERT INTO DataPoint "
                       "(Did, DName, NamePath, ItemPath, ItemName, ItemDescription) "
                       "VALUES (?, ?, ?, ?, ?, ?);");
        for (const auto& point : points) {
            sqlite3_reset(stmt.stmt);
            sqlite3_clear_bindings(stmt.stmt);
            sqlite3_bind_int(stmt.stmt, 1, device.id);
            bindText(stmt.stmt, 2, device.name);
            bindText(stmt.stmt, 3, point.namePath);
            bindText(stmt.stmt, 4, point.itemPath);
            bindText(stmt.stmt, 5, point.itemName);
            bindText(stmt.stmt, 6, point.itemDescription);
            stepDone(db_, stmt.stmt);
        }
        execute("COMMIT;");
    } catch (...) {
        execute("ROLLBACK;");
        throw;
    }
}

std::vector<Device> SQLiteManager::getDevices() const {
    Statement stmt(db_,
                   "SELECT Id, Name, IpAddr, Port, SampleInterval, Status, Model, ModelVersion, DriverPath "
                   "FROM Device ORDER BY Id;");

    std::vector<Device> devices;
    while (sqlite3_step(stmt.stmt) == SQLITE_ROW) {
        Device device;
        device.id = sqlite3_column_int(stmt.stmt, 0);
        device.name = columnText(stmt.stmt, 1);
        device.ip = columnText(stmt.stmt, 2);
        device.port = sqlite3_column_int(stmt.stmt, 3);
        device.sampleIntervalMs = sqlite3_column_int(stmt.stmt, 4);
        device.status = sqlite3_column_int(stmt.stmt, 5);
        device.model = columnText(stmt.stmt, 6);
        device.modelVersion = columnText(stmt.stmt, 7);
        device.driverPath = columnText(stmt.stmt, 8);
        devices.push_back(device);
    }

    return devices;
}

std::vector<DataPoint> SQLiteManager::getDataPointsByDeviceName(const std::string& deviceName) const {
    Statement stmt(db_,
                   "SELECT Id, Did, DName, NamePath, ItemPath, ItemName, ItemDescription "
                   "FROM DataPoint WHERE DName = ? ORDER BY Id;");
    bindText(stmt.stmt, 1, deviceName);

    std::vector<DataPoint> points;
    while (sqlite3_step(stmt.stmt) == SQLITE_ROW) {
        DataPoint point;
        point.id = sqlite3_column_int(stmt.stmt, 0);
        point.deviceId = sqlite3_column_int(stmt.stmt, 1);
        point.deviceName = columnText(stmt.stmt, 2);
        point.namePath = columnText(stmt.stmt, 3);
        point.itemPath = columnText(stmt.stmt, 4);
        point.itemName = columnText(stmt.stmt, 5);
        point.itemDescription = columnText(stmt.stmt, 6);
        points.push_back(point);
    }

    return points;
}

void SQLiteManager::insertResources(const std::vector<ResourceRecord>& records) {
    execute("BEGIN TRANSACTION;");
    try {
        Statement stmt(db_,
                       "INSERT INTO Resources "
                       "(DeviceName, DataItemNamePath, Data, DevTime, SvrTime, DataItemPath, DataItemName, DataItemDescription) "
                       "VALUES (?, ?, ?, ?, ?, ?, ?, ?);");

        for (const auto& record : records) {
            sqlite3_reset(stmt.stmt);
            sqlite3_clear_bindings(stmt.stmt);
            bindText(stmt.stmt, 1, record.deviceName);
            bindText(stmt.stmt, 2, record.dataItemNamePath);
            bindText(stmt.stmt, 3, record.data);
            sqlite3_bind_double(stmt.stmt, 4, record.devTime);
            sqlite3_bind_double(stmt.stmt, 5, record.svrTime);
            bindText(stmt.stmt, 6, record.dataItemPath);
            bindText(stmt.stmt, 7, record.dataItemName);
            bindText(stmt.stmt, 8, record.dataItemDescription);
            stepDone(db_, stmt.stmt);
        }
        execute("COMMIT;");
    } catch (...) {
        execute("ROLLBACK;");
        throw;
    }
}

std::vector<ResourceRecord> SQLiteManager::getLatestResources(const std::string& deviceName) const {
    Statement stmt(db_,
                   "SELECT DeviceName, DataItemNamePath, Data, DevTime, SvrTime, "
                   "DataItemPath, DataItemName, DataItemDescription "
                   "FROM Resources WHERE DeviceName = ? "
                   "ORDER BY SvrTime DESC, Id DESC LIMIT 100;");
    bindText(stmt.stmt, 1, deviceName);

    std::vector<ResourceRecord> records;
    while (sqlite3_step(stmt.stmt) == SQLITE_ROW) {
        ResourceRecord record;
        record.deviceName = columnText(stmt.stmt, 0);
        record.dataItemNamePath = columnText(stmt.stmt, 1);
        record.data = columnText(stmt.stmt, 2);
        record.devTime = sqlite3_column_double(stmt.stmt, 3);
        record.svrTime = sqlite3_column_double(stmt.stmt, 4);
        record.dataItemPath = columnText(stmt.stmt, 5);
        record.dataItemName = columnText(stmt.stmt, 6);
        record.dataItemDescription = columnText(stmt.stmt, 7);
        records.push_back(record);
    }

    return records;
}

void SQLiteManager::openDatabase() {
    const auto parentPath = std::filesystem::path(databasePath_).parent_path();
    if (!parentPath.empty()) {
        std::filesystem::create_directories(parentPath);
    }

    if (sqlite3_open(databasePath_.c_str(), &db_) != SQLITE_OK) {
        const std::string message = db_ != nullptr ? sqlite3_errmsg(db_) : "unknown sqlite error";
        throw std::runtime_error("failed to open database: " + message);
    }
}

void SQLiteManager::initializeSchema(const std::string& initSqlPath) {
    execute(readTextFile(initSqlPath));
    try {
        execute("ALTER TABLE Device ADD COLUMN Status INTEGER NOT NULL DEFAULT 1;");
    } catch (const std::exception& ex) {
        if (!isDuplicateColumnError(ex.what())) {
            throw;
        }
    }
}

void SQLiteManager::execute(const std::string& sql) const {
    char* errorMessage = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errorMessage) != SQLITE_OK) {
        std::string message = errorMessage != nullptr ? errorMessage : "unknown sqlite error";
        sqlite3_free(errorMessage);
        throw std::runtime_error(message);
    }
}

bool SQLiteManager::hasRows(const std::string& sql) const {
    Statement stmt(db_, sql);
    return sqlite3_step(stmt.stmt) == SQLITE_ROW;
}

