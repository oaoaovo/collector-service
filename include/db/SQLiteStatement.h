#pragma once

#include <sqlite3.h>

#include <string>

namespace db {

class SQLiteStatement {
public:
    SQLiteStatement(sqlite3* db, const std::string& sql);
    ~SQLiteStatement();

    SQLiteStatement(const SQLiteStatement&) = delete;
    SQLiteStatement& operator=(const SQLiteStatement&) = delete;

    void bindText(int index, const std::string& value);
    void bindInt(int index, int value);
    void bindDouble(int index, double value);
    bool stepRow();
    void stepDone();
    void reset();
    int columnInt(int index) const;
    double columnDouble(int index) const;
    std::string columnText(int index) const;

private:
    sqlite3* db_ = nullptr;
    sqlite3_stmt* stmt_ = nullptr;
};

} // namespace db
