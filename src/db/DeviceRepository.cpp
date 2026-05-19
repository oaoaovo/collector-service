#include "db/DeviceRepository.h"

#include "db/SQLiteConnection.h"
#include "db/SQLiteStatement.h"

#include <stdexcept>

namespace db {

namespace {

Device readDevice(SQLiteStatement& stmt) {
    Device device;
    device.id = stmt.columnInt(0);
    device.name = stmt.columnText(1);
    device.ip = stmt.columnText(2);
    device.port = stmt.columnInt(3);
    device.sampleIntervalMs = stmt.columnInt(4);
    device.status = stmt.columnInt(5);
    device.model = stmt.columnText(6);
    device.modelVersion = stmt.columnText(7);
    device.driverPath = stmt.columnText(8);
    return device;
}

} // namespace

DeviceRepository::DeviceRepository(SQLiteConnection& connection) : connection_(connection) {}

bool DeviceRepository::exists(const std::string& deviceName) const {
    SQLiteStatement stmt(connection_.handle(), "SELECT 1 FROM Device WHERE Name = ? LIMIT 1;");
    stmt.bindText(1, deviceName);
    return stmt.stepRow();
}

Device DeviceRepository::getByName(const std::string& deviceName) const {
    SQLiteStatement stmt(connection_.handle(),
                         "SELECT Id, Name, IpAddr, Port, SampleInterval, Status, Model, ModelVersion, DriverPath "
                         "FROM Device WHERE Name = ? LIMIT 1;");
    stmt.bindText(1, deviceName);

    if (!stmt.stepRow()) {
        throw std::runtime_error("device not found: " + deviceName);
    }
    return readDevice(stmt);
}

void DeviceRepository::create(const DeviceUpsert& device) {
    SQLiteStatement stmt(connection_.handle(),
                         "INSERT INTO Device "
                         "(Name, IpAddr, Port, SampleInterval, Status, Model, ModelVersion, DriverPath) "
                         "VALUES (?, ?, ?, ?, ?, ?, ?, ?);");
    stmt.bindText(1, device.name);
    stmt.bindText(2, device.ip);
    stmt.bindInt(3, device.port);
    stmt.bindInt(4, device.sampleIntervalMs);
    stmt.bindInt(5, device.status);
    stmt.bindText(6, device.model);
    stmt.bindText(7, device.modelVersion);
    stmt.bindText(8, device.driverPath);
    stmt.stepDone();
}

void DeviceRepository::update(const DeviceUpsert& device, const std::string& targetName) {
    SQLiteStatement stmt(connection_.handle(),
                         "UPDATE Device SET Name = ?, IpAddr = ?, Port = ?, SampleInterval = ?, "
                         "Status = ?, Model = ?, ModelVersion = ?, DriverPath = ? WHERE Name = ?;");
    stmt.bindText(1, device.name);
    stmt.bindText(2, device.ip);
    stmt.bindInt(3, device.port);
    stmt.bindInt(4, device.sampleIntervalMs);
    stmt.bindInt(5, device.status);
    stmt.bindText(6, device.model);
    stmt.bindText(7, device.modelVersion);
    stmt.bindText(8, device.driverPath);
    stmt.bindText(9, targetName);
    stmt.stepDone();
}

void DeviceRepository::remove(const std::string& deviceName) {
    SQLiteStatement stmt(connection_.handle(), "DELETE FROM Device WHERE Name = ?;");
    stmt.bindText(1, deviceName);
    stmt.stepDone();
}

std::vector<Device> DeviceRepository::getAll() const {
    SQLiteStatement stmt(connection_.handle(),
                         "SELECT Id, Name, IpAddr, Port, SampleInterval, Status, Model, ModelVersion, DriverPath "
                         "FROM Device ORDER BY Id;");

    std::vector<Device> devices;
    while (stmt.stepRow()) {
        devices.push_back(readDevice(stmt));
    }
    return devices;
}

} // namespace db
