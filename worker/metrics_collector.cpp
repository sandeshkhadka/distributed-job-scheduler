#include "metrics_collector.hpp"
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/statvfs.h>
#include <unistd.h>

MetricsCollector::MetricsCollector() : first_(true) {}

MetricsCollector::CpuTimes MetricsCollector::read_cpu_times() {
    CpuTimes t{};
    std::ifstream f("/proc/stat");
    std::string line;
    if (std::getline(f, line)) {
        std::istringstream ss(line);
        std::string cpu;
        ss >> cpu;
        ss >> t.user >> t.nice >> t.system >> t.idle >> t.iowait >> t.irq >> t.softirq >> t.steal;
    }
    return t;
}

MetricsCollector::NetBytes MetricsCollector::read_net_bytes() {
    NetBytes n{};
    std::ifstream f("/proc/net/dev");
    std::string line;
    while (std::getline(f, line)) {
        if (line.find("eth0:") != std::string::npos || line.find("ens") != std::string::npos ||
            line.find("enp") != std::string::npos) {
            std::istringstream ss(line);
            std::string iface;
            ss >> iface;
            ss >> n.rx;
            for (int i = 0; i < 7; ++i) {
                unsigned long long ignore;
                ss >> ignore;
            }
            ss >> n.tx;
            break;
        }
    }
    if (n.rx == 0 && n.tx == 0) {
        f.clear();
        f.seekg(0);
        while (std::getline(f, line)) {
            if (line.find(":") != std::string::npos && line.find("lo:") == std::string::npos) {
                std::istringstream ss(line);
                std::string iface;
                ss >> iface;
                ss >> n.rx;
                for (int i = 0; i < 7; ++i) {
                    unsigned long long ignore;
                    ss >> ignore;
                }
                ss >> n.tx;
                break;
            }
        }
    }
    return n;
}

WorkerMetricsData MetricsCollector::collect() {
    WorkerMetricsData data;

    std::ifstream meminfo("/proc/meminfo");
    unsigned long long mem_total_kb = 0;
    unsigned long long mem_avail_kb = 0;
    std::string line;
    while (std::getline(meminfo, line)) {
        if (line.find("MemTotal:") == 0) {
            std::istringstream ss(line);
            std::string key;
            ss >> key >> mem_total_kb;
        } else if (line.find("MemAvailable:") == 0) {
            std::istringstream ss(line);
            std::string key;
            ss >> key >> mem_avail_kb;
        }
    }

    if (mem_total_kb > 0) {
        unsigned long long mem_used_kb = mem_total_kb - mem_avail_kb;
        data.memory_total_mb = static_cast<double>(mem_total_kb) / 1024.0;
        data.memory_used_mb = static_cast<double>(mem_used_kb) / 1024.0;
        data.memory_percent = (static_cast<double>(mem_used_kb) / mem_total_kb) * 100.0;
    }

    std::ifstream loadavg("/proc/loadavg");
    if (std::getline(loadavg, line)) {
        std::istringstream ss(line);
        ss >> data.load_avg_1m;
    }

    struct statvfs sv;
    if (statvfs("/", &sv) == 0) {
        unsigned long long total = static_cast<unsigned long long>(sv.f_blocks) * sv.f_frsize;
        unsigned long long avail = static_cast<unsigned long long>(sv.f_bfree) * sv.f_frsize;
        unsigned long long used = total - avail;
        data.disk_total_mb = static_cast<double>(total) / (1024.0 * 1024.0);
        data.disk_used_mb = static_cast<double>(used) / (1024.0 * 1024.0);
        if (total > 0)
            data.disk_percent = (static_cast<double>(used) / total) * 100.0;
    }

    CpuTimes curr_cpu = read_cpu_times();
    if (!first_) {
        unsigned long long prev_idle = prev_cpu_.idle + prev_cpu_.iowait;
        unsigned long long curr_idle = curr_cpu.idle + curr_cpu.iowait;
        unsigned long long prev_total = prev_cpu_.user + prev_cpu_.nice + prev_cpu_.system +
                                        prev_cpu_.idle + prev_cpu_.iowait + prev_cpu_.irq +
                                        prev_cpu_.softirq + prev_cpu_.steal;
        unsigned long long curr_total = curr_cpu.user + curr_cpu.nice + curr_cpu.system +
                                        curr_cpu.idle + curr_cpu.iowait + curr_cpu.irq +
                                        curr_cpu.softirq + curr_cpu.steal;
        unsigned long long total_diff = curr_total - prev_total;
        unsigned long long idle_diff = curr_idle - prev_idle;
        if (total_diff > 0) {
            data.cpu_percent = 100.0 * (1.0 - static_cast<double>(idle_diff) / total_diff);
        }
    }
    prev_cpu_ = curr_cpu;

    NetBytes curr_net = read_net_bytes();
    if (!first_) {
        if (curr_net.rx >= prev_net_.rx)
            data.rx_bytes_per_sec = static_cast<double>(curr_net.rx - prev_net_.rx);
        if (curr_net.tx >= prev_net_.tx)
            data.tx_bytes_per_sec = static_cast<double>(curr_net.tx - prev_net_.tx);
    }
    prev_net_ = curr_net;

    first_ = false;
    return data;
}
