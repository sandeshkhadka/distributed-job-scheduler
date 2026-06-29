#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct EbpfSnapshot {
    int64_t job_id;
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

class EbpfMonitor {
  public:
    EbpfMonitor();
    ~EbpfMonitor();

    bool load_and_attach(const std::string& bpf_obj_path,
                         int job_id,
                         const std::string& cgroup_path,
                         int target_pid);
    EbpfSnapshot poll();
    void detach();

  private:
    struct bpf_object* obj_;
    struct bpf_program* prog_;
    struct bpf_link* link_;
    int fd_syscall_count_;
    int fd_io_bytes_;
    int fd_net_bytes_;
    int fd_target_cgroup_;
    int job_id_;
    std::string cgroup_path_;
    int target_pid_;
    int nr_cpus_;

    int64_t read_cgroup_u64(const std::string& file);
    int64_t lookup_percpu_sum(int fd, unsigned int key);
};
