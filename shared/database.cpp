#include "database.h"
#include "sqlite_db.hpp"
#include <iostream>

Database::Database(const std::string& db_name) {
    // create table for jobs
    SqliteDatabase::init(db_name);
    std::string query = "CREATE TABLE IF NOT EXISTS jobs ("
                        "id INTEGER PRIMARY KEY "
                        ", payload TEXT NOT NULL"
                        ", status TEXT NOT NULL CHECK (status IN (\"not started\", \"ongoing\", "
                        "\"completed\", \"terminated\"))"
                        "DEFAULT 'not started'"
                        ", client_id INTEGER NOT NULL"
                        ", created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                        ", updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                        ");";
    SqliteDatabase::instance().execute(query);

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

int Database::insert_job(const std::string& payload, int client_id) {
    std::string query = "INSERT INTO jobs (payload, client_id) VALUES (\"" + payload + "\", " +
                        std::to_string(client_id) + ")";
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
    j.payload = argv[1];
    j.status = argv[2];
    j.client_id = std::stoi(argv[3]);
    j.created_at = argv[4];
    j.updated_at = argv[4];
    job->emplace_back(j);
    return 0;
}

int create_job_cb(void* container, int argc, char** argv, char** col_name) {
    int* id = static_cast<int*>(container);
    std::cout << "create_job_cb: id = " << argv[0] << std::endl;
    *id = std::stoi(argv[0]);
    return 0;
}

void Database::insert_worker_job(const WorkerJob& worker_job) {
    std::string query = "INSERT INTO job_worker (job_id, worker_id) VALUES (" +
                        std::to_string(worker_job.job_id) + ", " +
                        std::to_string(worker_job.worker_id) + ");";
    SqliteDatabase::instance().execute(query);
}

int Database::insert_worker(const Worker& worker) {
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

void Database::update_worker_status(int worker_id, const std::string& status) {
    std::string query = "UPDATE workers SET status = '" + status +
                        "' WHERE id = " + std::to_string(worker_id) + ";";
    SqliteDatabase::instance().execute(query);
}

std::vector<Job> Database::get_worker_jobs(int worker_id) {
    std::string query = "SELECT j.id, j.payload FROM jobs j "
                        "JOIN job_worker jw ON j.id = jw.job_id "
                        "WHERE jw.worker_id = " +
                        std::to_string(worker_id) + ";";
    std::vector<Job> jobs;
    SqliteDatabase::instance().execute(query, get_job_cb, &jobs);
    return jobs;
}

Worker Database::get_worker_by_id(int worker_id) {
    std::string query = "SELECT * FROM workers WHERE id = " + std::to_string(worker_id) + ";";
    std::vector<Worker> workers;
    SqliteDatabase::instance().execute(query, get_worker_cb, &workers);
    return workers.empty() ? Worker() : workers[0];
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

int Database::insert_client(const Client& client) {
    std::string query = "INSERT INTO clients (name, status) VALUES ('" + client.name + "', '" +
                        client.status + "')";

    int id;
    SqliteDatabase::instance().execute(query);
    id = SqliteDatabase::instance().last_insert_rowid();
    return id;
}

int Database::get_registered_worker_id() {
    std::string create_active_workers = "SELECT * FROM workers;";
    int worker_id{-1};
    SqliteDatabase::instance().execute(
        create_active_workers, get_registered_worker_id_cb, &worker_id);

    return worker_id;
}

int get_registered_worker_id_cb(void* data, int argc, char** argv, char** col_name) {
    int* worker_id = static_cast<int*>(data);
    *worker_id = atoi(argv[1]); // 0 => id, 1 => server_id of the worker
    return 0;
}

int Database::get_active_client() {
    std::string select = "SELECT id FROM clients ORDER BY id DESC LIMIT 1;";
    int id = -1;
    SqliteDatabase::instance().execute(select, get_active_client_callback, &id);
    return id;
}

int get_active_client_callback(void* data, int, char** values, char**) {
    int* id = static_cast<int*>(data);
    *id = atoi(values[0]);
    return 0;
}
