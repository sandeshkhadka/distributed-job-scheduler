#include "database.h"
#include "sqlite_db.hpp"

Database::Database(const std::string& db_name) {
    SqliteDatabase::init(db_name);
    std::string query = "CREATE TABLE IF NOT EXISTS jobs ("
                        "id INTEGER PRIMARY KEY,"
                        "payload TEXT NOT NULL);";
    SqliteDatabase::instance().execute(query);
    // create table for jobs
}
