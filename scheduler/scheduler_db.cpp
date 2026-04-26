#include "scheduler_db.h"
#include "sqlite_db.hpp"
#include <vector>

int get_job_cb(void* container, int argc, char** argv, char** col_name) {
    std::vector<Job>* job = static_cast<std::vector<Job>*>(container);
    for (int i = 0; i < argc; i++) {
        job->emplace_back(Job{std::stoi(argv[0]), argv[1]});
    }
    return 0;
}

SchedulerDatabase::SchedulerDatabase() : Database("scheduler.db") {
    // create client table
    // create workter table
    // create client worker join table
    // create worker details table
}

// void SchedulerDatabase::insert_job(const std::string& payload) {
//     std::string query = "INSERT INTO jobs (payload) VALUES (\"" + payload + "\")";
//     SqliteDatabase::instance().execute(query);
// }

// Job SchedulerDatabase::get_jobs_by_id(int id) {
//     std::string query = "SELECT * FROM jobs WHERE id = " + std::to_string(id);
//     std::vector<Job> jobs;
//     SqliteDatabase::instance().execute(query, get_job_cb, &jobs);
//     if (jobs.empty()) {
//         return Job{0, ""};
//     }
//     return jobs[0];
// }
