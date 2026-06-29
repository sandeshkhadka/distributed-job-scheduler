#include "ebpf_monitor.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <string>
#include <thread>
#include <unistd.h>

static std::string derive_bpf_path(const char* argv0) {
    std::string path = argv0 ? argv0 : "./djs-ebpf-monitor";
    auto slash = path.rfind('/');
    if (slash != std::string::npos) {
        path = path.substr(0, slash + 1) + "bpf/job_monitor.bpf.o";
    } else {
        path = "bpf/job_monitor.bpf.o";
    }
    return path;
}

static void print_json(const EbpfSnapshot& snap) {
    fprintf(stdout,
            "{\"job_id\":%ld,\"ts\":%.3f,"
            "\"syscall_read\":%ld,\"syscall_write\":%ld,\"syscall_openat\":%ld,"
            "\"io_read_bytes\":%ld,\"io_write_bytes\":%ld,"
            "\"net_tx_bytes\":%ld,\"net_rx_bytes\":%ld,"
            "\"cpu_us\":%ld,\"mem_bytes\":%ld}\n",
            snap.job_id,
            snap.timestamp,
            snap.syscall_read_count,
            snap.syscall_write_count,
            snap.syscall_openat_count,
            snap.io_read_bytes,
            snap.io_write_bytes,
            snap.net_tx_bytes,
            snap.net_rx_bytes,
            snap.cpu_usage_us,
            snap.mem_current_bytes);
    fflush(stdout);
}

int main(int argc, char* argv[]) {
    int job_id = 0;
    int target_pid = -1;
    std::string cgroup_path;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--job-id") == 0 && i + 1 < argc)
            job_id = atoi(argv[++i]);
        else if (strcmp(argv[i], "--pid") == 0 && i + 1 < argc)
            target_pid = atoi(argv[++i]);
        else if (strcmp(argv[i], "--cgroup-path") == 0 && i + 1 < argc)
            cgroup_path = argv[++i];
    }

    if (job_id <= 0 || target_pid <= 0 || cgroup_path.empty()) {
        fprintf(stderr, "usage: djs-ebpf-monitor --job-id N --pid N --cgroup-path /path\n");
        return 1;
    }

    std::string bpf_path = derive_bpf_path(argv[0]);

    EbpfMonitor monitor;
    if (!monitor.load_and_attach(bpf_path, job_id, cgroup_path, target_pid)) {
        fprintf(stderr, "eBPF setup failed -- running without eBPF monitoring\n");
        // Return error so the orchestrator knows monitoring is unavailable
        return 2;
    }

    while (true) {
        if (kill(target_pid, 0) < 0 && errno == ESRCH) {
            // Job finished: emit final snapshot and exit
            auto snap = monitor.poll();
            print_json(snap);
            break;
        }

        auto snap = monitor.poll();
        print_json(snap);

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return 0;
}
