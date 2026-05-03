#include "worker_db.hpp"
#include "sqlite_db.hpp"
#include <string>

WorkerDatabase::WorkerDatabase() : Database("workers.db") {}

// insert id received from scheduler instead of null
int WorkerDatabase::insert_worker(const Worker& worker) {
    std::string query = "INSERT INTO workers (id, cpu_cores, mem_size, disk_size, cpu_freq, os, "
                        "kernel_version, name, "
                        "status) VALUES (" +
                        std::to_string(worker.id) + ", " + std::to_string(worker.cpu_cores) + ", " +
                        std::to_string(worker.mem_size) + ", " + std::to_string(worker.disk_size) +
                        ", " + std::to_string(worker.cpu_freq) + ", " + "'" + worker.os + "', " +
                        "'" + worker.kernel_version + "', " + "'" + worker.name + "', " + "'" +
                        worker.status + "'" + ");";
    int id;
    SqliteDatabase::instance().execute(query);
    id = SqliteDatabase::instance().last_insert_rowid();
    return id;
}

int WorkerDatabase::insert_job(const Job& job) {
    std::string query = "INSERT INTO jobs (id, payload, status, client_id) VALUES (" +
                        std::to_string(job.id) + ", \"" + job.payload + "\", \"" + job.status +
                        "\", " + std::to_string(job.client_id) + ")";
    int id;
    SqliteDatabase::instance().execute(query);
    id = SqliteDatabase::instance().last_insert_rowid();
    return id;
}
