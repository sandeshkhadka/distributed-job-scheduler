#include "pg_scheduler_db.hpp"
#include "database.h"
#include "logger.h"
#include "pg_db.hpp"
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>

using Logger = DJS::Logger;

namespace {

int get_worker_cb(void* data, int argc, char** argv, char**) {
    auto* workers = static_cast<std::vector<Worker>*>(data);
    Worker w;
    w.id = std::stoi(argv[0]);
    w.cpu_cores = std::stoi(argv[1]);
    w.mem_size = std::stof(argv[2]);
    w.disk_size = std::stof(argv[3]);
    w.cpu_freq = std::stof(argv[4]);
    w.os = argv[5] ? argv[5] : "";
    w.name = argv[6] ? argv[6] : "";
    w.status = argv[7] ? argv[7] : "";
    workers->push_back(w);
    return 0;
}

int int64_cb(void* data, int argc, char** argv, char**) {
    auto* val = static_cast<int64_t*>(data);
    if (argc >= 1 && argv[0])
        *val = std::stoll(argv[0]);
    return 0;
}

int avg_duration_cb(void* data, int argc, char** argv, char**) {
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

int token_exists_cb(void* data, int argc, char** argv, char**) {
    auto* count = static_cast<int*>(data);
    if (argc >= 1 && argv[0])
        *count = std::stoi(argv[0]);
    return 0;
}

int token_id_cb(void* data, int argc, char** argv, char**) {
    auto* id = static_cast<int*>(data);
    if (argc >= 1 && argv[0])
        *id = std::stoi(argv[0]);
    return 0;
}

} // anonymous namespace

PgSchedulerDatabase::PgSchedulerDatabase() {
    PgDatabase::init();
    PgDatabase::instance().apply_migrations("migrations/001_init.sql");
}

int PgSchedulerDatabase::insert_job(const std::string& job_type,
                                    const std::string& params,
                                    int client_id) {
    std::string q = "INSERT INTO jobs (job_type, params, client_id) VALUES ('" + job_type + "', '" +
                    params + "', " + std::to_string(client_id) + ") RETURNING id";
    return static_cast<int>(PgDatabase::instance().execute_insert(q));
}

Job PgSchedulerDatabase::get_job_by_id(int job_id) {
    std::string q = "SELECT * FROM jobs WHERE id = " + std::to_string(job_id);
    std::vector<Job> jobs;
    PgDatabase::instance().execute(q, get_job_cb, &jobs);
    return jobs.empty() ? Job{} : jobs[0];
}

std::vector<Job> PgSchedulerDatabase::get_jobs_by_status(const std::string& status) {
    std::string q = "SELECT * FROM jobs WHERE status = '" + status + "'";
    std::vector<Job> jobs;
    PgDatabase::instance().execute(q, get_job_cb, &jobs);
    return jobs;
}

Worker PgSchedulerDatabase::get_worker_by_id(int worker_id) {
    std::string q = "SELECT * FROM workers WHERE id = " + std::to_string(worker_id);
    std::vector<Worker> workers;
    PgDatabase::instance().execute(q, get_worker_cb, &workers);
    return workers.empty() ? Worker{} : workers[0];
}

int PgSchedulerDatabase::insert_worker(const Worker& worker) {
    std::string q = "INSERT INTO workers (cpu_cores, mem_size, disk_size, cpu_freq, os, name, "
                    "status) VALUES (" +
                    std::to_string(worker.cpu_cores) + ", " + std::to_string(worker.mem_size) +
                    ", " + std::to_string(worker.disk_size) + ", " +
                    std::to_string(worker.cpu_freq) + ", '" + worker.os + "', '" + worker.name +
                    "', '" + worker.status + "') RETURNING id";
    return static_cast<int>(PgDatabase::instance().execute_insert(q));
}

void PgSchedulerDatabase::insert_worker_job(const WorkerJob& worker_job) {
    std::string q = "INSERT INTO job_worker (job_id, worker_id) VALUES (" +
                    std::to_string(worker_job.job_id) + ", " +
                    std::to_string(worker_job.worker_id) + ")";
    PgDatabase::instance().execute(q);
}

void PgSchedulerDatabase::update_job_status(int job_id, const std::string& status) {
    std::string q = "UPDATE jobs SET status = '" + status +
                    "', updated_at = CURRENT_TIMESTAMP WHERE id = " + std::to_string(job_id);
    PgDatabase::instance().execute(q);
}

void PgSchedulerDatabase::save_job_result(int job_id,
                                          bool success,
                                          const std::string& message,
                                          const std::string& artifact_url) {
    std::string q = "INSERT INTO job_results (job_id, success, message, artifact_url) VALUES (" +
                    std::to_string(job_id) + ", " + std::to_string(success ? 1 : 0) + ", '" +
                    message + "', '" + artifact_url +
                    "') ON CONFLICT (job_id) DO UPDATE SET success = EXCLUDED.success, message = "
                    "EXCLUDED.message, artifact_url = EXCLUDED.artifact_url";
    PgDatabase::instance().execute(q);
}

void PgSchedulerDatabase::save_worker_metrics(int worker_id,
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
    std::string q =
        "INSERT INTO worker_metrics (worker_id, cpu_percent, memory_percent, memory_used_mb, "
        "memory_total_mb, disk_used_mb, disk_total_mb, disk_percent, rx_bytes_per_sec, "
        "tx_bytes_per_sec, load_avg_1m, active_jobs) VALUES (" +
        std::to_string(worker_id) + ", " + std::to_string(cpu_percent) + ", " +
        std::to_string(memory_percent) + ", " + std::to_string(memory_used_mb) + ", " +
        std::to_string(memory_total_mb) + ", " + std::to_string(disk_used_mb) + ", " +
        std::to_string(disk_total_mb) + ", " + std::to_string(disk_percent) + ", " +
        std::to_string(rx_bytes_per_sec) + ", " + std::to_string(tx_bytes_per_sec) + ", " +
        std::to_string(load_avg_1m) + ", " + std::to_string(active_jobs) +
        ") "
        "ON CONFLICT (worker_id) DO UPDATE SET cpu_percent = EXCLUDED.cpu_percent, "
        "memory_percent = EXCLUDED.memory_percent, memory_used_mb = EXCLUDED.memory_used_mb, "
        "memory_total_mb = EXCLUDED.memory_total_mb, disk_used_mb = EXCLUDED.disk_used_mb, "
        "disk_total_mb = EXCLUDED.disk_total_mb, disk_percent = EXCLUDED.disk_percent, "
        "rx_bytes_per_sec = EXCLUDED.rx_bytes_per_sec, tx_bytes_per_sec = "
        "EXCLUDED.tx_bytes_per_sec, "
        "load_avg_1m = EXCLUDED.load_avg_1m, active_jobs = EXCLUDED.active_jobs";
    PgDatabase::instance().execute(q);
}

int64_t PgSchedulerDatabase::compute_duration_ms(const std::string& started_at) {
    std::string q = "SELECT EXTRACT(EPOCH FROM NOW() - '" + started_at + "'::timestamp) * 1000";
    int64_t result = 0;
    PgDatabase::instance().execute(q, int64_cb, &result);
    return result;
}

bool PgSchedulerDatabase::is_token_valid(const std::string& token, const std::string& token_type) {
    std::string q = "SELECT COUNT(*) FROM auth_tokens WHERE token = '" + token +
                    "' AND token_type = '" + token_type + "' AND active = 1";
    int count = 0;
    try {
        PgDatabase::instance().execute(q, token_exists_cb, &count);
    } catch (const std::exception& e) {
        Logger::Error("is_token_valid query failed: " + std::string(e.what()));
        return false;
    }
    return count > 0;
}

int PgSchedulerDatabase::get_token_id(const std::string& token) {
    std::string q = "SELECT id FROM auth_tokens WHERE token = '" + token + "'";
    int id = -1;
    PgDatabase::instance().execute(q, token_id_cb, &id);
    return id;
}

void PgSchedulerDatabase::record_client_token_usage(int client_id, int token_id) {
    std::string q =
        "INSERT INTO client_token_usage (client_id, token_id) VALUES (" +
        std::to_string(client_id) + ", " + std::to_string(token_id) +
        ") "
        "ON CONFLICT (client_id, token_id) DO UPDATE SET last_used_at = CURRENT_TIMESTAMP";
    PgDatabase::instance().execute(q);
}

std::map<std::string, double> PgSchedulerDatabase::get_avg_durations() {
    std::string q =
        "SELECT job_type, AVG(duration_ms) FROM job_runtime_analytics GROUP BY job_type";
    std::map<std::string, double> result;
    PgDatabase::instance().execute(q, avg_duration_cb, &result);
    return result;
}

std::map<std::string, JobTypeSpikes> PgSchedulerDatabase::get_avg_spikes() {
    std::string q = "SELECT job_type, AVG(cpu_spike_percent), AVG(memory_spike_percent), COUNT(*) "
                    "FROM job_runtime_analytics GROUP BY job_type";
    std::map<std::string, JobTypeSpikes> result;
    PgDatabase::instance().execute(q, avg_spikes_cb, &result);
    return result;
}

void PgSchedulerDatabase::record_job_snapshot(int job_id, double start_cpu, double start_memory) {
    std::string q = "INSERT INTO job_runtime_snapshots (job_id, started_at, start_cpu_percent, "
                    "start_memory_percent, "
                    "peak_cpu_percent, peak_memory_percent) VALUES (" +
                    std::to_string(job_id) + ", CURRENT_TIMESTAMP, " + std::to_string(start_cpu) +
                    ", " + std::to_string(start_memory) + ", " + std::to_string(start_cpu) + ", " +
                    std::to_string(start_memory) +
                    ") "
                    "ON CONFLICT (job_id) DO UPDATE SET started_at = EXCLUDED.started_at, "
                    "start_cpu_percent = EXCLUDED.start_cpu_percent, start_memory_percent = "
                    "EXCLUDED.start_memory_percent, "
                    "peak_cpu_percent = EXCLUDED.peak_cpu_percent, peak_memory_percent = "
                    "EXCLUDED.peak_memory_percent";
    PgDatabase::instance().execute(q);
}

JobSnapshot PgSchedulerDatabase::get_job_snapshot(int job_id) {
    std::string q = "SELECT started_at, start_cpu_percent, start_memory_percent, peak_cpu_percent, "
                    "peak_memory_percent "
                    "FROM job_runtime_snapshots WHERE job_id = " +
                    std::to_string(job_id);
    JobSnapshot snap{};
    PgDatabase::instance().execute(q, job_snapshot_cb, &snap);
    return snap;
}

void PgSchedulerDatabase::update_worker_job_peaks(int worker_id,
                                                  double cpu_percent,
                                                  double memory_percent) {
    std::string q = "SELECT j.id FROM jobs j "
                    "JOIN job_worker jw ON j.id = jw.job_id "
                    "WHERE jw.worker_id = " +
                    std::to_string(worker_id) + " AND j.status IN ('started', 'ongoing')";
    std::vector<int> job_ids;
    PgDatabase::instance().execute(q, int_vector_cb, &job_ids);
    for (int jid : job_ids) {
        std::string upd =
            "UPDATE job_runtime_snapshots SET peak_cpu_percent = GREATEST(peak_cpu_percent, " +
            std::to_string(cpu_percent) +
            "), peak_memory_percent = GREATEST(peak_memory_percent, " +
            std::to_string(memory_percent) + ") WHERE job_id = " + std::to_string(jid);
        PgDatabase::instance().execute(upd);
    }
}

void PgSchedulerDatabase::save_job_analytics(int job_id,
                                             const std::string& job_type,
                                             int64_t duration_ms,
                                             double cpu_spike,
                                             double memory_spike,
                                             double peak_cpu,
                                             double peak_memory) {
    std::string q = "INSERT INTO job_runtime_analytics (job_id, job_type, duration_ms, "
                    "cpu_spike_percent, memory_spike_percent, "
                    "peak_cpu_percent, peak_memory_percent) VALUES (" +
                    std::to_string(job_id) + ", '" + job_type + "', " +
                    std::to_string(duration_ms) + ", " + std::to_string(cpu_spike) + ", " +
                    std::to_string(memory_spike) + ", " + std::to_string(peak_cpu) + ", " +
                    std::to_string(peak_memory) + ")";
    PgDatabase::instance().execute(q);

    std::string cleanup = "DELETE FROM job_runtime_analytics WHERE id NOT IN ("
                          "SELECT id FROM job_runtime_analytics WHERE job_type = '" +
                          job_type + "' ORDER BY recorded_at DESC LIMIT 10) AND job_type = '" +
                          job_type + "'";
    PgDatabase::instance().execute(cleanup);
}

int PgSchedulerDatabase::insert_client(const Client& client) {
    std::string q = "INSERT INTO clients (name, status) VALUES ('" + client.name + "', '" +
                    client.status + "') RETURNING id";
    return static_cast<int>(PgDatabase::instance().execute_insert(q));
}

std::optional<WorkerMetricsBrief> PgSchedulerDatabase::get_worker_latest_metrics(int worker_id) {
    std::string q = "SELECT cpu_percent, memory_percent, memory_used_mb, memory_total_mb "
                    "FROM worker_metrics WHERE worker_id = " +
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
    PgDatabase::instance().execute(q, cb, &ctx);
    if (ctx.second)
        return m;
    return std::nullopt;
}

void PgSchedulerDatabase::save_job_ebpf_metrics(int64_t job_id,
                                                int64_t worker_id,
                                                double timestamp,
                                                int64_t syscall_read_count,
                                                int64_t syscall_write_count,
                                                int64_t syscall_openat_count,
                                                int64_t io_read_bytes,
                                                int64_t io_write_bytes,
                                                int64_t net_tx_bytes,
                                                int64_t net_rx_bytes,
                                                int64_t cpu_usage_us,
                                                int64_t mem_current_bytes) {
    std::string q = "INSERT INTO job_ebpf_metrics (job_id, worker_id, recorded_at, "
                    "syscall_read_count, syscall_write_count, syscall_openat_count, "
                    "io_read_bytes, io_write_bytes, net_tx_bytes, net_rx_bytes, "
                    "cpu_usage_us, mem_current_bytes) VALUES (" +
                    std::to_string(job_id) + ", " + std::to_string(worker_id) + ", " +
                    std::to_string(timestamp) + ", " + std::to_string(syscall_read_count) + ", " +
                    std::to_string(syscall_write_count) + ", " +
                    std::to_string(syscall_openat_count) + ", " + std::to_string(io_read_bytes) +
                    ", " + std::to_string(io_write_bytes) + ", " + std::to_string(net_tx_bytes) +
                    ", " + std::to_string(net_rx_bytes) + ", " + std::to_string(cpu_usage_us) +
                    ", " + std::to_string(mem_current_bytes) + ")";
    PgDatabase::instance().execute(q);
}

namespace {

int ebpf_timeseries_cb(void* data, int argc, char** argv, char**) {
    auto* points = static_cast<std::vector<PgSchedulerDatabase::EbpfTimeseriesPoint>*>(data);
    PgSchedulerDatabase::EbpfTimeseriesPoint p{};
    if (argc >= 1 && argv[0])
        p.timestamp = std::stod(argv[0]);
    if (argc >= 2 && argv[1])
        p.syscall_read_count = std::stoll(argv[1]);
    if (argc >= 3 && argv[2])
        p.syscall_write_count = std::stoll(argv[2]);
    if (argc >= 4 && argv[3])
        p.syscall_openat_count = std::stoll(argv[3]);
    if (argc >= 5 && argv[4])
        p.io_read_bytes = std::stoll(argv[4]);
    if (argc >= 6 && argv[5])
        p.io_write_bytes = std::stoll(argv[5]);
    if (argc >= 7 && argv[6])
        p.net_tx_bytes = std::stoll(argv[6]);
    if (argc >= 8 && argv[7])
        p.net_rx_bytes = std::stoll(argv[7]);
    if (argc >= 9 && argv[8])
        p.cpu_usage_us = std::stoll(argv[8]);
    if (argc >= 10 && argv[9])
        p.mem_current_bytes = std::stoll(argv[9]);
    points->push_back(p);
    return 0;
}

} // anonymous namespace

std::vector<PgSchedulerDatabase::EbpfTimeseriesPoint>
PgSchedulerDatabase::get_job_timeseries(int job_id, int limit) {
    std::string q = "SELECT recorded_at, syscall_read_count, syscall_write_count, "
                    "syscall_openat_count, io_read_bytes, io_write_bytes, "
                    "net_tx_bytes, net_rx_bytes, cpu_usage_us, mem_current_bytes "
                    "FROM job_ebpf_metrics WHERE job_id = " +
                    std::to_string(job_id) + " ORDER BY recorded_at ASC LIMIT " +
                    std::to_string(limit);
    std::vector<EbpfTimeseriesPoint> points;
    PgDatabase::instance().execute(q, ebpf_timeseries_cb, &points);
    return points;
}
