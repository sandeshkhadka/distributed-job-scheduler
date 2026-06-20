#include "scheduler_db.h"
#include "logger.h"
#include "sqlite_db.hpp"
#include <cstdint>
#include <map>

using Logger = DJS::Logger;

int string_cb(void* data, int argc, char** argv, char** col_name);
int int64_cb(void* data, int argc, char** argv, char** col_name);
int avg_duration_cb(void* data, int argc, char** argv, char** col_name);

SchedulerDatabase::SchedulerDatabase() : Database("scheduler.db") {
    // create client table
    std::string create_client_table = "CREATE TABLE IF NOT EXISTS clients ("
                                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                      "name TEXT NOT NULL, " // hostname
                                      "status TEXT NOT NULL"
                                      ");";
    SqliteDatabase::instance().execute(create_client_table);

    // create worker table
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

    std::string create_auth_tokens = "CREATE TABLE IF NOT EXISTS auth_tokens ("
                                     "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                     "token TEXT NOT NULL UNIQUE, "
                                     "description TEXT, "
                                     "token_type TEXT NOT NULL DEFAULT 'client' "
                                     "  CHECK (token_type IN ('client', 'worker')), "
                                     "active INTEGER NOT NULL DEFAULT 1, "
                                     "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                                     ");";
    SqliteDatabase::instance().execute(create_auth_tokens);

    std::string create_usage = "CREATE TABLE IF NOT EXISTS client_token_usage ("
                               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                               "client_id INTEGER NOT NULL, "
                               "token_id INTEGER NOT NULL, "
                               "last_used_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, "
                               "FOREIGN KEY (client_id) REFERENCES clients(id), "
                               "FOREIGN KEY (token_id) REFERENCES auth_tokens(id), "
                               "UNIQUE(client_id, token_id)"
                               ");";
    SqliteDatabase::instance().execute(create_usage);

    std::string create_results = "CREATE TABLE IF NOT EXISTS job_results ("
                                 "job_id INTEGER PRIMARY KEY, "
                                 "success INTEGER NOT NULL, "
                                 "message TEXT, "
                                 "artifact_url TEXT, "
                                 "metrics_json TEXT, "
                                 "completed_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, "
                                 "FOREIGN KEY (job_id) REFERENCES jobs(id)"
                                 ");";
    SqliteDatabase::instance().execute(create_results);

    std::string create_timing = "CREATE TABLE IF NOT EXISTS job_type_timing ("
                                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                "job_type TEXT NOT NULL, "
                                "duration_ms INTEGER NOT NULL, "
                                "recorded_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                                ");";
    SqliteDatabase::instance().execute(create_timing);

    std::string create_starts = "CREATE TABLE IF NOT EXISTS job_starts ("
                                "job_id INTEGER PRIMARY KEY, "
                                "started_at TEXT NOT NULL"
                                ");";
    SqliteDatabase::instance().execute(create_starts);

    std::string create_metrics = "CREATE TABLE IF NOT EXISTS worker_metrics ("
                                 "worker_id INTEGER PRIMARY KEY, "
                                 "cpu_percent REAL, "
                                 "memory_percent REAL, "
                                 "memory_used_mb REAL, "
                                 "memory_total_mb REAL, "
                                 "disk_used_mb REAL, "
                                 "disk_total_mb REAL, "
                                 "disk_percent REAL, "
                                 "rx_bytes_per_sec REAL, "
                                 "tx_bytes_per_sec REAL, "
                                 "load_avg_1m REAL, "
                                 "active_jobs INTEGER, "
                                 "reported_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, "
                                 "FOREIGN KEY (worker_id) REFERENCES workers(id)"
                                 ");";
    SqliteDatabase::instance().execute(create_metrics);
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

bool SchedulerDatabase::is_token_valid(const std::string& token, const std::string& token_type) {
    std::string query = "SELECT COUNT(*) FROM auth_tokens "
                        "WHERE token = '" +
                        token +
                        "' "
                        "AND token_type = '" +
                        token_type +
                        "' "
                        "AND active = 1";
    int count = 0;
    try {
        SqliteDatabase::instance().execute(query, token_exists_cb, &count);
    } catch (const std::exception& e) {
        Logger::Error("is_token_valid query failed: " + std::string(e.what()));
        return false;
    }
    return count > 0;
}

int SchedulerDatabase::get_token_id(const std::string& token) {
    std::string query = "SELECT id FROM auth_tokens WHERE token = '" + token + "'";
    int id = -1;
    SqliteDatabase::instance().execute(query, token_id_cb, &id);
    return id;
}

void SchedulerDatabase::record_client_token_usage(int client_id, int token_id) {
    std::string query = "INSERT INTO client_token_usage (client_id, token_id) VALUES (" +
                        std::to_string(client_id) + ", " + std::to_string(token_id) +
                        ") "
                        "ON CONFLICT(client_id, token_id) "
                        "DO UPDATE SET last_used_at = CURRENT_TIMESTAMP";
    SqliteDatabase::instance().execute(query);
}

void SchedulerDatabase::update_job_status(int job_id, const std::string& status) {
    std::string query = "UPDATE jobs SET status = '" + status +
                        "', updated_at = CURRENT_TIMESTAMP WHERE id = " + std::to_string(job_id) +
                        ";";
    SqliteDatabase::instance().execute(query);
}

void SchedulerDatabase::save_job_result(int job_id,
                                        bool success,
                                        const std::string& message,
                                        const std::string& artifact_url) {
    std::string query =
        "INSERT OR REPLACE INTO job_results (job_id, success, message, artifact_url) "
        "VALUES (" +
        std::to_string(job_id) + ", " + std::to_string(success ? 1 : 0) + ", '" + message + "', '" +
        artifact_url + "')";
    SqliteDatabase::instance().execute(query);
}

void SchedulerDatabase::save_worker_metrics(int worker_id,
                                            double cpu_percent,
                                            double memory_percent,
                                            double memory_used_mb,
                                            double memory_total_mb,
                                            double disk_used_mb,
                                            double disk_total_mb,
                                            double disk_percent,
                                            double rx_bytes_per_sec,
                                            double tx_bytes_per_sec,
                                            double load_avg_1m,
                                            int active_jobs) {
    std::string query =
        "INSERT OR REPLACE INTO worker_metrics "
        "(worker_id, cpu_percent, memory_percent, memory_used_mb, memory_total_mb, "
        "disk_used_mb, disk_total_mb, disk_percent, rx_bytes_per_sec, tx_bytes_per_sec, "
        "load_avg_1m, active_jobs) "
        "VALUES (" +
        std::to_string(worker_id) + ", " + std::to_string(cpu_percent) + ", " +
        std::to_string(memory_percent) + ", " + std::to_string(memory_used_mb) + ", " +
        std::to_string(memory_total_mb) + ", " + std::to_string(disk_used_mb) + ", " +
        std::to_string(disk_total_mb) + ", " + std::to_string(disk_percent) + ", " +
        std::to_string(rx_bytes_per_sec) + ", " + std::to_string(tx_bytes_per_sec) + ", " +
        std::to_string(load_avg_1m) + ", " + std::to_string(active_jobs) + ")";
    SqliteDatabase::instance().execute(query);
}

void SchedulerDatabase::record_job_started_at(int job_id) {
    std::string query = "INSERT OR REPLACE INTO job_starts (job_id, started_at) "
                        "VALUES (" +
                        std::to_string(job_id) + ", CURRENT_TIMESTAMP)";
    SqliteDatabase::instance().execute(query);
}

std::string SchedulerDatabase::get_job_started_at(int job_id) {
    std::string query =
        "SELECT started_at FROM job_starts WHERE job_id = " + std::to_string(job_id);
    std::string result;
    SqliteDatabase::instance().execute(query, string_cb, &result);
    return result;
}

int64_t SchedulerDatabase::compute_duration_ms(const std::string& started_at) {
    std::string query =
        "SELECT CAST((julianday('now') - julianday('" + started_at + "')) * 86400000 AS INTEGER)";
    int64_t result = 0;
    SqliteDatabase::instance().execute(query, int64_cb, &result);
    return result;
}

void SchedulerDatabase::save_job_timing(const std::string& job_type, int64_t duration_ms) {
    std::string query = "INSERT INTO job_type_timing (job_type, duration_ms) VALUES ('" + job_type +
                        "', " + std::to_string(duration_ms) + ")";
    SqliteDatabase::instance().execute(query);

    std::string cleanup = "DELETE FROM job_type_timing WHERE id NOT IN ("
                          "SELECT id FROM job_type_timing WHERE job_type = '" +
                          job_type +
                          "' ORDER BY recorded_at DESC LIMIT 10) "
                          "AND job_type = '" +
                          job_type + "'";
    SqliteDatabase::instance().execute(cleanup);
}

std::map<std::string, double> SchedulerDatabase::get_avg_durations() {
    std::string query = "SELECT job_type, AVG(duration_ms) FROM job_type_timing GROUP BY job_type";
    std::map<std::string, double> result;
    SqliteDatabase::instance().execute(query, avg_duration_cb, &result);
    return result;
}

int string_cb(void* data, int argc, char** argv, char** col_name) {
    auto* str = static_cast<std::string*>(data);
    if (argc >= 1 && argv[0])
        *str = argv[0];
    return 0;
}

int int64_cb(void* data, int argc, char** argv, char** col_name) {
    auto* val = static_cast<int64_t*>(data);
    if (argc >= 1 && argv[0])
        *val = std::stoll(argv[0]);
    return 0;
}

int avg_duration_cb(void* data, int argc, char** argv, char** col_name) {
    auto* map = static_cast<std::map<std::string, double>*>(data);
    if (argc >= 2 && argv[0] && argv[1])
        (*map)[argv[0]] = std::stod(argv[1]);
    return 0;
}

int token_exists_cb(void* data, int argc, char** argv, char** col_name) {
    int* count = static_cast<int*>(data);
    *count = atoi(argv[0]);
    return 0;
}

int token_id_cb(void* data, int argc, char** argv, char** col_name) {
    int* id = static_cast<int*>(data);
    *id = atoi(argv[0]);
    return 0;
}
