#include "scheduler_db.h"
#include "database.hpp"

SchedulerDatabase::SchedulerDatabase() {
    Database::init("scheduler.db");
    std::string query = "CREATE TABLE IF NOT EXISTS jobs ("
                        "id INTEGER PRIMARY KEY,"
                        "payload TEXT NOT NULL);";
    Database::instance().execute(query);
}

void SchedulerDatabase::insert_job(const std::string& payload) {
    std::string query = "INSERT INTO jobs (payload) VALUES (\"" + payload + "\")";
    Database::instance().execute(query);
}
