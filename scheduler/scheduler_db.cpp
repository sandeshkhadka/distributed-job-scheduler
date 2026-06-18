#include "scheduler_db.h"
#include "sqlite_db.hpp"

SchedulerDatabase::SchedulerDatabase() : Database("scheduler.db") {
    // create client table
    std::string create_client_table = "CREATE TABLE IF NOT EXISTS clients ("
                                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                      "name TEXT NOT NULL, " // hostname
                                      "status TEXT NOT NULL"
                                      ");";
    SqliteDatabase::instance().execute(create_client_table);

    // create workter table
    std::string create_worker_table = "CREATE TABLE IF NOT EXISTS workers ("
                                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                      "cpu_cores INTEGER NOT NULL, "
                                      "mem_size REAL NOT NULL, "
                                      "disk_size REAL NOT NULL, "
                                      "cpu_freq REAL NOT NULL, "
                                      "os TEXT NOT NULL, "
                                      "name TEXT NOT NULL, "
                                      "status TEXT NOT NULL"
                                      ");";
    SqliteDatabase::instance().execute(create_worker_table);

    // create job worker join table
    std::string create_job_worker_table = "CREATE TABLE IF NOT EXISTS job_worker ("
                                          "job_id INTEGER NOT NULL, "
                                          "worker_id INTEGER NOT NULL, "
                                          "PRIMARY KEY (job_id, worker_id), "
                                          "FOREIGN KEY (job_id) REFERENCES jobs(id), "
                                          "FOREIGN KEY (worker_id) REFERENCES workers(id)"
                                          ");";
    SqliteDatabase::instance().execute(create_job_worker_table);
}

void SchedulerDatabase::insert_worker_job(const WorkerJob& worker_job) {
    std::string query = "INSERT INTO job_worker (job_id, worker_id) VALUES (" +
                        std::to_string(worker_job.job_id) + ", " +
                        std::to_string(worker_job.worker_id) + ");";
    SqliteDatabase::instance().execute(query);
}

int SchedulerDatabase::insert_worker(const Worker& worker) {
    std::string query = "INSERT INTO workers (cpu_cores, mem_size, disk_size, cpu_freq, os, name, "
                        "status) VALUES (" +
                        std::to_string(worker.cpu_cores) + ", " + std::to_string(worker.mem_size) +
                        ", " + std::to_string(worker.disk_size) + ", " +
                        std::to_string(worker.cpu_freq) + ", " + "'" + worker.os + "', " + "'" +
                        worker.name + "', " + "'" + worker.status + "'" + ");";
    int id;
    SqliteDatabase::instance().execute(query);
    id = SqliteDatabase::instance().last_insert_rowid();
    return id;
}

void SchedulerDatabase::update_worker_status(int worker_id, const std::string& status) {
    std::string query = "UPDATE workers SET status = '" + status +
                        "' WHERE id = " + std::to_string(worker_id) + ";";
    SqliteDatabase::instance().execute(query);
}

std::vector<Job> SchedulerDatabase::get_worker_jobs(int worker_id) {
    std::string query = "SELECT j.* FROM jobs j "
                        "JOIN job_worker jw ON j.id = jw.job_id "
                        "WHERE jw.worker_id = " +
                        std::to_string(worker_id) + ";";
    std::vector<Job> jobs;
    SqliteDatabase::instance().execute(query, get_job_cb, &jobs);
    return jobs;
}

Worker SchedulerDatabase::get_worker_by_id(int worker_id) {
    std::string query = "SELECT * FROM workers WHERE id = " + std::to_string(worker_id) + ";";
    std::vector<Worker> workers;
    SqliteDatabase::instance().execute(query, get_worker_cb, &workers);
    return workers.empty() ? Worker() : workers[0];
}

int insert_worker_cb(void* data, int argc, char** argv, char** col_name) {
    int* id = static_cast<int*>(data);
    *id = atoi(argv[0]);
    return 0;
}

int get_worker_cb(void* data, int argc, char** argv, char** col_name) {
    std::vector<Worker>* workers = static_cast<std::vector<Worker>*>(data);
    Worker worker;
    worker.id = atoi(argv[0]);
    worker.cpu_cores = atoi(argv[1]);
    worker.mem_size = atoi(argv[2]);
    worker.disk_size = atoi(argv[3]);
    worker.cpu_freq = atoi(argv[4]);
    worker.os = argv[5];
    worker.name = argv[6];
    worker.status = argv[7];
    workers->push_back(worker);
    return 0;
}

int SchedulerDatabase::insert_client(const Client& client) {
    std::string query = "INSERT INTO clients (name, status) VALUES ('" + client.name + "', '" +
                        client.status + "')";

    int id;
    SqliteDatabase::instance().execute(query);
    id = SqliteDatabase::instance().last_insert_rowid();
    return id;
}
