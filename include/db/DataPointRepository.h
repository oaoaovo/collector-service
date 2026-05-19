#pragma once

#include "Models.h"

#include <string>
#include <vector>

namespace db {

class SQLiteConnection;

class DataPointRepository {
public:
    explicit DataPointRepository(SQLiteConnection& connection);

    void deleteByDeviceName(const std::string& deviceName);
    void renameDevice(const std::string& oldName, const std::string& newName);
    void replaceForDevice(const Device& device, const std::vector<DataPoint>& points);
    std::vector<DataPoint> getByDeviceName(const std::string& deviceName) const;

private:
    SQLiteConnection& connection_;
};

} // namespace db
