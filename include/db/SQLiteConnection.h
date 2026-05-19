#pragma once

#include <sqlite3.h>

#include <mutex>
#include <string>

namespace db {

class SQLiteConnection {
public:
    explicit SQLiteConnection(std::string databasePath);
    ~SQLiteConnection();

    SQLiteConnection(const SQLiteConnection&) = delete;
    SQLiteConnection& operator=(const SQLiteConnection&) = delete;

    sqlite3* handle() const;
    std::recursive_mutex& mutex() const;
    void execute(const std::string& sql) const;

private:
    std::string databasePath_;
    sqlite3* db_ = nullptr;
    mutable std::recursive_mutex mutex_;
};

} // namespace db
