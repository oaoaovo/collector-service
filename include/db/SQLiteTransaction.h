#pragma once

#include "db/SQLiteConnection.h"

namespace db {

class SQLiteTransaction {
public:
    explicit SQLiteTransaction(SQLiteConnection& connection);
    ~SQLiteTransaction();

    SQLiteTransaction(const SQLiteTransaction&) = delete;
    SQLiteTransaction& operator=(const SQLiteTransaction&) = delete;

    void commit();

private:
    SQLiteConnection& connection_;
    bool committed_ = false;
};

} // namespace db
