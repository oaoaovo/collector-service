#include "db/ResourceRepository.h"

#include "db/SQLiteConnection.h"
#include "db/SQLiteStatement.h"

namespace db {

ResourceRepository::ResourceRepository(SQLiteConnection& connection) : connection_(connection) {}

void ResourceRepository::insert(const std::vector<ResourceRecord>& records) {
    SQLiteStatement stmt(connection_.handle(),
                         "INSERT INTO Resources "
                         "(DeviceName, DataItemNamePath, Data, DevTime, SvrTime, DataItemPath, DataItemName, DataItemDescription) "
                         "VALUES (?, ?, ?, ?, ?, ?, ?, ?);");

    for (const auto& record : records) {
        stmt.reset();
        stmt.bindText(1, record.deviceName);
        stmt.bindText(2, record.dataItemNamePath);
        stmt.bindText(3, record.data);
        stmt.bindDouble(4, record.devTime);
        stmt.bindDouble(5, record.svrTime);
        stmt.bindText(6, record.dataItemPath);
        stmt.bindText(7, record.dataItemName);
        stmt.bindText(8, record.dataItemDescription);
        stmt.stepDone();
    }
}

std::vector<ResourceRecord> ResourceRepository::getLatest(const std::string& deviceName) const {
    SQLiteStatement stmt(connection_.handle(),
                         "SELECT DeviceName, DataItemNamePath, Data, DevTime, SvrTime, "
                         "DataItemPath, DataItemName, DataItemDescription "
                         "FROM Resources WHERE DeviceName = ? "
                         "ORDER BY SvrTime DESC, Id DESC LIMIT 100;");
    stmt.bindText(1, deviceName);

    std::vector<ResourceRecord> records;
    while (stmt.stepRow()) {
        ResourceRecord record;
        record.deviceName = stmt.columnText(0);
        record.dataItemNamePath = stmt.columnText(1);
        record.data = stmt.columnText(2);
        record.devTime = stmt.columnDouble(3);
        record.svrTime = stmt.columnDouble(4);
        record.dataItemPath = stmt.columnText(5);
        record.dataItemName = stmt.columnText(6);
        record.dataItemDescription = stmt.columnText(7);
        records.push_back(record);
    }
    return records;
}

} // namespace db
