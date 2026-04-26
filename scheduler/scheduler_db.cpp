#include "scheduler_db.h"
#include "database.hpp"
#include <iostream>
#include <vector>

int get_job_cb(void* container, int argc, char** argv, char** col_name) {
    std::vector<Job>* job = static_cast<std::vector<Job>*>(container);
    for (int i = 0; i < argc; i++) {
        job->emplace_back(Job{std::stoi(argv[0]), argv[1]});
    }
    return 0;
}

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

Job SchedulerDatabase::get_jobs_by_id(int id) {
    std::string query = "SELECT * FROM jobs WHERE id = " + std::to_string(id);
    std::vector<Job> jobs;
    Database::instance().execute(query, get_job_cb, &jobs);
    if (jobs.empty()) {
        return Job{0, ""};
    }
    return jobs[0];
}
