#include "db/ResourceRetentionService.h"

#include "db/SQLiteConnection.h"
#include "db/SQLiteStatement.h"

namespace db {

ResourceRetentionService::ResourceRetentionService(SQLiteConnection& connection) : connection_(connection) {}

void ResourceRetentionService::cleanupResources(int maxRowsPerDevice) {
    if (maxRowsPerDevice <= 0) {
        return;
    }

    SQLiteStatement stmt(connection_.handle(),
                         "DELETE FROM Resources WHERE Id IN ("
                         "SELECT Id FROM ("
                         "SELECT Id, ROW_NUMBER() OVER (PARTITION BY DeviceName ORDER BY SvrTime DESC, Id DESC) AS rn "
                         "FROM Resources"
                         ") WHERE rn > ?"
                         ");");
    stmt.bindInt(1, maxRowsPerDevice);
    stmt.stepDone();
}

void ResourceRetentionService::cleanupResourcesForDevice(const std::string& deviceName, int maxRows) {
    if (maxRows <= 0 || deviceName.empty()) {
        return;
    }

    SQLiteStatement stmt(connection_.handle(),
                         "DELETE FROM Resources WHERE Id IN ("
                         "SELECT Id FROM ("
                         "SELECT Id, ROW_NUMBER() OVER (ORDER BY SvrTime DESC, Id DESC) AS rn "
                         "FROM Resources WHERE DeviceName = ?"
                         ") WHERE rn > ?"
                         ");");
    stmt.bindText(1, deviceName);
    stmt.bindInt(2, maxRows);
    stmt.stepDone();
}

} // namespace db
