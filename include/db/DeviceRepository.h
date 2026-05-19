#pragma once

#include "Models.h"

#include <string>
#include <vector>

namespace db {

class SQLiteConnection;

class DeviceRepository {
public:
    explicit DeviceRepository(SQLiteConnection& connection);

    bool exists(const std::string& deviceName) const;
    Device getByName(const std::string& deviceName) const;
    void create(const DeviceUpsert& device);
    void update(const DeviceUpsert& device, const std::string& targetName);
    void remove(const std::string& deviceName);
    std::vector<Device> getAll() const;

private:
    SQLiteConnection& connection_;
};

} // namespace db
