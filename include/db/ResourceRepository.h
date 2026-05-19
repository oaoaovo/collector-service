#pragma once

#include "Models.h"

#include <string>
#include <vector>

namespace db {

class SQLiteConnection;

class ResourceRepository {
public:
    explicit ResourceRepository(SQLiteConnection& connection);

    void insert(const std::vector<ResourceRecord>& records);
    std::vector<ResourceRecord> getLatest(const std::string& deviceName) const;

private:
    SQLiteConnection& connection_;
};

} // namespace db
