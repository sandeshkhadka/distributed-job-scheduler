#include "scheduler_db.h"
#include "logger.h"
#include "sqlite_db.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

using Logger = DJS::Logger;

int int64_cb(void* data, int argc, char** argv, char** col_name);
int avg_duration_cb(void* data, int argc, char** argv, char** col_name);
int avg_spikes_cb(void* data, int argc, char** argv, char** col_name);

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

    std::string create_snapshots = "CREATE TABLE IF NOT EXISTS job_runtime_snapshots ("
                                   "job_id INTEGER PRIMARY KEY, "
                                   "started_at TEXT NOT NULL, "
                                   "start_cpu_percent REAL DEFAULT 0, "
                                   "start_memory_percent REAL DEFAULT 0, "
                                   "peak_cpu_percent REAL DEFAULT 0, "
                                   "peak_memory_percent REAL DEFAULT 0"
                                   ");";
    SqliteDatabase::instance().execute(create_snapshots);

    std::string create_analytics = "CREATE TABLE IF NOT EXISTS job_runtime_analytics ("
                                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                   "job_id INTEGER NOT NULL, "
                                   "job_type TEXT NOT NULL, "
                                   "duration_ms INTEGER NOT NULL, "
                                   "cpu_spike_percent REAL DEFAULT 0, "
                                   "memory_spike_percent REAL DEFAULT 0, "
                                   "peak_cpu_percent REAL DEFAULT 0, "
                                   "peak_memory_percent REAL DEFAULT 0, "
                                   "recorded_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                                   ");";
    SqliteDatabase::instance().execute(create_analytics);

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

int64_t SchedulerDatabase::compute_duration_ms(const std::string& started_at) {
    std::string query =
        "SELECT CAST((julianday('now') - julianday('" + started_at + "')) * 86400000 AS INTEGER)";
    int64_t result = 0;
    SqliteDatabase::instance().execute(query, int64_cb, &result);
    return result;
}

std::map<std::string, double> SchedulerDatabase::get_avg_durations() {
    std::string query =
        "SELECT job_type, AVG(duration_ms) FROM job_runtime_analytics GROUP BY job_type";
    std::map<std::string, double> result;
    SqliteDatabase::instance().execute(query, avg_duration_cb, &result);
    return result;
}

void SchedulerDatabase::record_job_snapshot(int job_id, double start_cpu, double start_memory) {
    std::string query = "INSERT OR REPLACE INTO job_runtime_snapshots "
                        "(job_id, started_at, start_cpu_percent, start_memory_percent, "
                        "peak_cpu_percent, peak_memory_percent) "
                        "VALUES (" +
                        std::to_string(job_id) + ", CURRENT_TIMESTAMP, " +
                        std::to_string(start_cpu) + ", " + std::to_string(start_memory) + ", " +
                        std::to_string(start_cpu) + ", " + std::to_string(start_memory) + ")";
    SqliteDatabase::instance().execute(query);
}

JobSnapshot SchedulerDatabase::get_job_snapshot(int job_id) {
    std::string query = "SELECT started_at, start_cpu_percent, start_memory_percent, "
                        "peak_cpu_percent, peak_memory_percent "
                        "FROM job_runtime_snapshots WHERE job_id = " +
                        std::to_string(job_id);
    JobSnapshot snap{};
    SqliteDatabase::instance().execute(query, job_snapshot_cb, &snap);
    return snap;
}

void SchedulerDatabase::update_job_peak_metrics(int job_id,
                                                double cpu_percent,
                                                double memory_percent) {
    std::string query = "UPDATE job_runtime_snapshots SET "
                        "peak_cpu_percent = MAX(peak_cpu_percent, " +
                        std::to_string(cpu_percent) +
                        "), "
                        "peak_memory_percent = MAX(peak_memory_percent, " +
                        std::to_string(memory_percent) +
                        ") "
                        "WHERE job_id = " +
                        std::to_string(job_id);
    SqliteDatabase::instance().execute(query);
}

void SchedulerDatabase::update_worker_job_peaks(int worker_id,
                                                double cpu_percent,
                                                double memory_percent) {
    std::string query = "SELECT j.id FROM jobs j "
                        "JOIN job_worker jw ON j.id = jw.job_id "
                        "WHERE jw.worker_id = " +
                        std::to_string(worker_id) + " AND j.status IN ('started', 'ongoing')";
    std::vector<int> job_ids;
    SqliteDatabase::instance().execute(query, int_vector_cb, &job_ids);
    for (int jid : job_ids) {
        update_job_peak_metrics(jid, cpu_percent, memory_percent);
    }
}

void SchedulerDatabase::save_job_analytics(int job_id,
                                           const std::string& job_type,
                                           int64_t duration_ms,
                                           double cpu_spike,
                                           double memory_spike,
                                           double peak_cpu,
                                           double peak_memory) {
    std::string query = "INSERT INTO job_runtime_analytics "
                        "(job_id, job_type, duration_ms, cpu_spike_percent, memory_spike_percent, "
                        "peak_cpu_percent, peak_memory_percent) "
                        "VALUES (" +
                        std::to_string(job_id) + ", '" + job_type + "', " +
                        std::to_string(duration_ms) + ", " + std::to_string(cpu_spike) + ", " +
                        std::to_string(memory_spike) + ", " + std::to_string(peak_cpu) + ", " +
                        std::to_string(peak_memory) + ")";
    SqliteDatabase::instance().execute(query);

    std::string cleanup = "DELETE FROM job_runtime_analytics WHERE id NOT IN ("
                          "SELECT id FROM job_runtime_analytics WHERE job_type = '" +
                          job_type +
                          "' ORDER BY recorded_at DESC LIMIT 10) "
                          "AND job_type = '" +
                          job_type + "'";
    SqliteDatabase::instance().execute(cleanup);
}

std::map<std::string, JobTypeSpikes> SchedulerDatabase::get_avg_spikes() {
    std::string query = "SELECT job_type, AVG(cpu_spike_percent), AVG(memory_spike_percent), "
                        "COUNT(*) FROM job_runtime_analytics GROUP BY job_type";
    std::map<std::string, JobTypeSpikes> result;
    SqliteDatabase::instance().execute(query, avg_spikes_cb, &result);
    return result;
}

std::optional<SchedulerDatabase::WorkerMetricsBrief>
SchedulerDatabase::get_worker_latest_metrics(int worker_id) {
    std::string query = "SELECT cpu_percent, memory_percent, memory_used_mb, memory_total_mb "
                        "FROM worker_metrics "
                        "WHERE worker_id = " +
                        std::to_string(worker_id);
    WorkerMetricsBrief m{};
    int found = 0;
    auto cb = [](void* data, int argc, char** argv, char**) -> int {
        auto* ctx = static_cast<std::pair<WorkerMetricsBrief*, int>*>(data);
        if (argc >= 4 && argv[0] && argv[1] && argv[2] && argv[3]) {
            ctx->first->cpu_percent = std::stod(argv[0]);
            ctx->first->memory_percent = std::stod(argv[1]);
            ctx->first->memory_used_mb = std::stod(argv[2]);
            ctx->first->memory_total_mb = std::stod(argv[3]);
            ctx->second = 1;
        }
        return 0;
    };
    std::pair<WorkerMetricsBrief*, int> ctx{&m, 0};
    SqliteDatabase::instance().execute(query, cb, &ctx);
    if (ctx.second)
        return m;
    return std::nullopt;
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

int avg_spikes_cb(void* data, int argc, char** argv, char**) {
    auto* map = static_cast<std::map<std::string, JobTypeSpikes>*>(data);
    if (argc >= 4 && argv[0] && argv[1] && argv[2] && argv[3])
        (*map)[argv[0]] = {std::stod(argv[1]), std::stod(argv[2]), std::stoi(argv[3])};
    return 0;
}

int int_vector_cb(void* data, int argc, char** argv, char**) {
    auto* vec = static_cast<std::vector<int>*>(data);
    if (argc >= 1 && argv[0])
        vec->push_back(std::stoi(argv[0]));
    return 0;
}

int job_snapshot_cb(void* data, int argc, char** argv, char**) {
    auto* snap = static_cast<JobSnapshot*>(data);
    if (argc >= 1 && argv[0])
        snap->started_at = argv[0];
    if (argc >= 2 && argv[1])
        snap->start_cpu_percent = std::stod(argv[1]);
    if (argc >= 3 && argv[2])
        snap->start_memory_percent = std::stod(argv[2]);
    if (argc >= 4 && argv[3])
        snap->peak_cpu_percent = std::stod(argv[3]);
    if (argc >= 5 && argv[4])
        snap->peak_memory_percent = std::stod(argv[4]);
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
