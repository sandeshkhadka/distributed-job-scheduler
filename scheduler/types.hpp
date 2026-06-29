#pragma once
#include <map>
#include <optional>
#include <string>
#include <vector>

struct Worker {
    int id;
    int cpu_cores;
    float mem_size;
    float disk_size;
    float cpu_freq;
    std::string os;
    std::string name;
    std::string status;
};

struct WorkerJob {
    int worker_id;
    int job_id;
};

struct Client {
    std::string name;
    std::string status;
};

struct AuthToken {
    int id;
    std::string token;
    std::string description;
    std::string token_type;
    bool active;
    std::string created_at;
};

struct JobSnapshot {
    std::string started_at;
    double start_cpu_percent;
    double start_memory_percent;
    double peak_cpu_percent;
    double peak_memory_percent;
};

struct JobTypeSpikes {
    double avg_cpu_spike;
    double avg_memory_spike;
    int sample_count;
};

struct WorkerMetricsBrief {
    double cpu_percent;
    double memory_percent;
    double memory_used_mb;
    double memory_total_mb;
};
