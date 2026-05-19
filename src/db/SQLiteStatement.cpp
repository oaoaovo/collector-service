#include "db/SQLiteStatement.h"

#include <stdexcept>

namespace db {

SQLiteStatement::SQLiteStatement(sqlite3* db, const std::string& sql) : db_(db) {
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db_));
    }
}

SQLiteStatement::~SQLiteStatement() {
    if (stmt_ != nullptr) {
        sqlite3_finalize(stmt_);
    }
}

void SQLiteStatement::bindText(int index, const std::string& value) {
    if (sqlite3_bind_text(stmt_, index, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        throw std::runtime_error("failed to bind text parameter");
    }
}

void SQLiteStatement::bindInt(int index, int value) {
    if (sqlite3_bind_int(stmt_, index, value) != SQLITE_OK) {
        throw std::runtime_error("failed to bind int parameter");
    }
}

void SQLiteStatement::bindDouble(int index, double value) {
    if (sqlite3_bind_double(stmt_, index, value) != SQLITE_OK) {
        throw std::runtime_error("failed to bind double parameter");
    }
}

bool SQLiteStatement::stepRow() {
    return sqlite3_step(stmt_) == SQLITE_ROW;
}

void SQLiteStatement::stepDone() {
    const int result = sqlite3_step(stmt_);
    if (result != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db_));
    }
}

void SQLiteStatement::reset() {
    sqlite3_reset(stmt_);
    sqlite3_clear_bindings(stmt_);
}

int SQLiteStatement::columnInt(int index) const {
    return sqlite3_column_int(stmt_, index);
}

double SQLiteStatement::columnDouble(int index) const {
    return sqlite3_column_double(stmt_, index);
}

std::string SQLiteStatement::columnText(int index) const {
    const auto* text = sqlite3_column_text(stmt_, index);
    return text != nullptr ? reinterpret_cast<const char*>(text) : "";
}

} // namespace db
