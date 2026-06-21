#pragma once
#include <cstdint>
#include <string>

struct WorkerMetricsData {
    double cpu_percent = 0.0;
    double memory_percent = 0.0;
    double memory_used_mb = 0.0;
    double memory_total_mb = 0.0;
    double disk_used_mb = 0.0;
    double disk_total_mb = 0.0;
    double disk_percent = 0.0;
    double rx_bytes_per_sec = 0.0;
    double tx_bytes_per_sec = 0.0;
    double load_avg_1m = 0.0;
};

class MetricsCollector {
  public:
    MetricsCollector();
    WorkerMetricsData collect();

  private:
    struct CpuTimes {
        unsigned long long user = 0;
        unsigned long long nice = 0;
        unsigned long long system = 0;
        unsigned long long idle = 0;
        unsigned long long iowait = 0;
        unsigned long long irq = 0;
        unsigned long long softirq = 0;
        unsigned long long steal = 0;
    };

    struct NetBytes {
        unsigned long long rx = 0;
        unsigned long long tx = 0;
    };

    CpuTimes prev_cpu_;
    NetBytes prev_net_;
    bool first_;

    static CpuTimes read_cpu_times();
    static NetBytes read_net_bytes();
};
