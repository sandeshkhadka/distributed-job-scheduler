#include "database.hpp"

void Database::execute(const std::string& query) {
    std::lock_guard<std::mutex> lock(_mutex);
    char* err_msg = nullptr;
    if (sqlite3_exec(_db, query.c_str(), nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::string error = err_msg ? err_msg : "Unknown error";
        sqlite3_free(err_msg);
        throw std::runtime_error(error);
    }
}
Database::Database(const std::string& db_path) {
    if (sqlite3_open(db_path.c_str(), &_db) != SQLITE_OK) {
        throw std::runtime_error("Cannot Open Database");
    }
    sqlite3_busy_timeout(_db, 5000);
    execute("PRAGMA journal_mode=WAL;");
}

Database::~Database() { sqlite3_close(_db); }

Database* Database::_instance = nullptr;
std::mutex Database::_init_mutex;
