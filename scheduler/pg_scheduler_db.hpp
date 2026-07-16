#pragma once
#include "database.h"
#include "idb.hpp"
#include "types.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

class PgSchedulerDatabase : public ISchedulerDatabase {
  public:
    PgSchedulerDatabase();

    // ISchedulerDatabase
    bool is_token_valid(const std::string& token, const std::string& token_type) override;
    std::optional<WorkerMetricsBrief> get_worker_latest_metrics(int worker_id) override;
    std::map<std::string, double> get_avg_durations() override;
    std::map<std::string, JobTypeSpikes> get_avg_spikes() override;

    int insert_job(const std::string& job_type, const std::string& params, int client_id);
    Job get_job_by_id(int job_id);
    std::vector<Job> get_jobs_by_status(const std::string& status);
    std::vector<Job> get_jobs_by_client_id(int client_id);

    struct JobResultInfo {
        int job_id;
        std::string status;
        bool success;
        std::string message;
        std::string artifact_url;
        std::string completed_at;
    };
    JobResultInfo get_job_result(int job_id);

    Worker get_worker_by_id(int worker_id);
    int insert_worker(const Worker& worker);
    void insert_worker_job(const WorkerJob& worker_job);
    void update_job_status(int job_id, const std::string& status);
    void save_job_result(int job_id,
                         bool success,
                         const std::string& message,
                         const std::string& artifact_url);
    void save_worker_metrics(int worker_id,
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
                             int active_jobs);
    int64_t compute_duration_ms(const std::string& started_at);
    void record_job_snapshot(int job_id, double start_cpu, double start_memory);
    JobSnapshot get_job_snapshot(int job_id);
    void update_worker_job_peaks(int worker_id, double cpu_percent, double memory_percent);
    void save_job_analytics(int job_id,
                            const std::string& job_type,
                            int64_t duration_ms,
                            double cpu_spike,
                            double memory_spike,
                            double peak_cpu,
                            double peak_memory);
    int insert_client(const Client& client);
    int get_token_id(const std::string& token);
    void record_client_token_usage(int client_id, int token_id);

    void save_job_ebpf_metrics(int64_t job_id,
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
                               int64_t mem_current_bytes);
    struct EbpfTimeseriesPoint {
        double timestamp;
        int64_t syscall_read_count;
        int64_t syscall_write_count;
        int64_t syscall_openat_count;
        int64_t io_read_bytes;
        int64_t io_write_bytes;
        int64_t net_tx_bytes;
        int64_t net_rx_bytes;
        int64_t cpu_usage_us;
        int64_t mem_current_bytes;
    };
    std::vector<EbpfTimeseriesPoint> get_job_timeseries(int job_id, int limit);
};
