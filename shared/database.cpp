#include "database.h"
#include "sqlite_db.hpp"
#include <iostream>

Database::Database(const std::string& db_name) {
    SqliteDatabase::init(db_name);
    std::string query = "CREATE TABLE IF NOT EXISTS jobs ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT"
                        ", job_type TEXT NOT NULL"
                        ", params TEXT NOT NULL DEFAULT ''"
                        ", status TEXT NOT NULL DEFAULT 'not started'"
                        ", client_id INTEGER NOT NULL"
                        ", created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                        ", updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                        ");";
    SqliteDatabase::instance().execute(query);
    // create table for jobs
}

int Database::insert_job(const std::string& job_type, const std::string& params, int client_id) {
    std::string query = "INSERT INTO jobs (job_type, params, client_id) VALUES ('" + job_type +
                        "', '" + params + "', " + std::to_string(client_id) + ")";
    int id;
    SqliteDatabase::instance().execute(query);
    id = SqliteDatabase::instance().last_insert_rowid();
    return id;
}

Job Database::get_job_by_id(int id) {
    std::string query = "SELECT * FROM jobs WHERE id = " + std::to_string(id);
    std::vector<Job> jobs;
    SqliteDatabase::instance().execute(query, get_job_cb, &jobs);
    if (jobs.empty()) {
        return Job{};
    }
    return jobs[0];
}
std::vector<Job> Database::get_job_by_client_id(int client_id) {
    std::string query = "SELECT * FROM jobs WHERE client_id = " + std::to_string(client_id);
    std::vector<Job> jobs;
    SqliteDatabase::instance().execute(query, get_job_cb, &jobs);
    return jobs;
}

std::vector<Job> Database::get_jobs_by_status(const std::string& status) {
    std::string query = "SELECT * FROM jobs WHERE status = \"" + status + "\"";
    std::vector<Job> jobs;
    SqliteDatabase::instance().execute(query, get_job_cb, &jobs);
    return jobs;
}

int get_job_cb(void* container, int argc, char** argv, char** col_name) {
    std::vector<Job>* job = static_cast<std::vector<Job>*>(container);
    Job j;
    j.id = std::stoi(argv[0]);
    j.job_type = argv[1];
    j.params = argv[2];
    j.status = argv[3];
    j.client_id = std::stoi(argv[4]);
    j.created_at = argv[5];
    j.updated_at = argv[6];
    job->emplace_back(j);
    return 0;
}

int create_job_cb(void* container, int argc, char** argv, char** col_name) {
    int* id = static_cast<int*>(container);
    std::cout << "create_job_cb: id = " << argv[0] << std::endl;
    *id = std::stoi(argv[0]);
    return 0;
}
