#include "db/SQLiteConnection.h"

#include <filesystem>
#include <stdexcept>
#include <utility>

namespace db {

SQLiteConnection::SQLiteConnection(std::string databasePath) : databasePath_(std::move(databasePath)) {
    const auto parentPath = std::filesystem::path(databasePath_).parent_path();
    if (!parentPath.empty()) {
        std::filesystem::create_directories(parentPath);
    }

    if (sqlite3_open(databasePath_.c_str(), &db_) != SQLITE_OK) {
        const std::string message = db_ != nullptr ? sqlite3_errmsg(db_) : "unknown sqlite error";
        throw std::runtime_error("failed to open database: " + message);
    }
}

SQLiteConnection::~SQLiteConnection() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

sqlite3* SQLiteConnection::handle() const {
    return db_;
}

std::recursive_mutex& SQLiteConnection::mutex() const {
    return mutex_;
}

void SQLiteConnection::execute(const std::string& sql) const {
    char* errorMessage = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errorMessage) != SQLITE_OK) {
        std::string message = errorMessage != nullptr ? errorMessage : "unknown sqlite error";
        sqlite3_free(errorMessage);
        throw std::runtime_error(message);
    }
}

} // namespace db
