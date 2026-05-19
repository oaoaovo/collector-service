#include "db/DataPointRepository.h"

#include "db/SQLiteConnection.h"
#include "db/SQLiteStatement.h"

namespace db {

DataPointRepository::DataPointRepository(SQLiteConnection& connection) : connection_(connection) {}

void DataPointRepository::deleteByDeviceName(const std::string& deviceName) {
    SQLiteStatement stmt(connection_.handle(), "DELETE FROM DataPoint WHERE DName = ?;");
    stmt.bindText(1, deviceName);
    stmt.stepDone();
}

void DataPointRepository::renameDevice(const std::string& oldName, const std::string& newName) {
    SQLiteStatement stmt(connection_.handle(), "UPDATE DataPoint SET DName = ? WHERE DName = ?;");
    stmt.bindText(1, newName);
    stmt.bindText(2, oldName);
    stmt.stepDone();
}

void DataPointRepository::replaceForDevice(const Device& device, const std::vector<DataPoint>& points) {
    deleteByDeviceName(device.name);

    SQLiteStatement stmt(connection_.handle(),
                         "INSERT INTO DataPoint "
                         "(Did, DName, NamePath, ItemPath, ItemName, ItemDescription) "
                         "VALUES (?, ?, ?, ?, ?, ?);");
    for (const auto& point : points) {
        stmt.reset();
        stmt.bindInt(1, device.id);
        stmt.bindText(2, device.name);
        stmt.bindText(3, point.namePath);
        stmt.bindText(4, point.itemPath);
        stmt.bindText(5, point.itemName);
        stmt.bindText(6, point.itemDescription);
        stmt.stepDone();
    }
}

std::vector<DataPoint> DataPointRepository::getByDeviceName(const std::string& deviceName) const {
    SQLiteStatement stmt(connection_.handle(),
                         "SELECT Id, Did, DName, NamePath, ItemPath, ItemName, ItemDescription "
                         "FROM DataPoint WHERE DName = ? ORDER BY Id;");
    stmt.bindText(1, deviceName);

    std::vector<DataPoint> points;
    while (stmt.stepRow()) {
        DataPoint point;
        point.id = stmt.columnInt(0);
        point.deviceId = stmt.columnInt(1);
        point.deviceName = stmt.columnText(2);
        point.namePath = stmt.columnText(3);
        point.itemPath = stmt.columnText(4);
        point.itemName = stmt.columnText(5);
        point.itemDescription = stmt.columnText(6);
        points.push_back(point);
    }
    return points;
}

} // namespace db
