#pragma once

#include <string>

namespace db {

class SQLiteConnection;

class ResourceRetentionService {
public:
    explicit ResourceRetentionService(SQLiteConnection& connection);

    void cleanupResources(int maxRowsPerDevice);
    void cleanupResourcesForDevice(const std::string& deviceName, int maxRows);

private:
    SQLiteConnection& connection_;
};

} // namespace db
