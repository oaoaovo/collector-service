#include "db/SQLiteTransaction.h"

namespace db {

SQLiteTransaction::SQLiteTransaction(SQLiteConnection& connection) : connection_(connection) {
    connection_.execute("BEGIN TRANSACTION;");
}

SQLiteTransaction::~SQLiteTransaction() {
    if (!committed_) {
        try {
            connection_.execute("ROLLBACK;");
        } catch (...) {
        }
    }
}

void SQLiteTransaction::commit() {
    connection_.execute("COMMIT;");
    committed_ = true;
}

} // namespace db
