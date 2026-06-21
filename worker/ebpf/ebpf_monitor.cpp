#include "ebpf_monitor.hpp"
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <linux/bpf.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

EbpfMonitor::EbpfMonitor()
    : obj_(nullptr), prog_(nullptr), link_(nullptr), fd_syscall_count_(-1), fd_io_bytes_(-1),
      fd_net_bytes_(-1), fd_target_cgroup_(-1), job_id_(0), target_pid_(-1), nr_cpus_(0) {
    nr_cpus_ = sysconf(_SC_NPROCESSORS_CONF);
    if (nr_cpus_ <= 0)
        nr_cpus_ = 1;
}

EbpfMonitor::~EbpfMonitor() { detach(); }

static int print_libbpf_log(enum libbpf_print_level level, const char* msg, va_list ap) {
    if (level > LIBBPF_WARN)
        return 0;
    vfprintf(stderr, msg, ap);
    return 0;
}

bool EbpfMonitor::load_and_attach(const std::string& bpf_obj_path,
                                  int job_id,
                                  const std::string& cgroup_path,
                                  int target_pid) {
    job_id_ = job_id;
    cgroup_path_ = cgroup_path;
    target_pid_ = target_pid;

    struct stat st;
    if (stat(cgroup_path.c_str(), &st) < 0) {
        fprintf(stderr, "cgroup path not found: %s\n", cgroup_path.c_str());
        return false;
    }
    __u64 cgroup_id = (__u64)st.st_ino;

    libbpf_set_print(print_libbpf_log);

    obj_ = bpf_object__open_file(bpf_obj_path.c_str(), nullptr);
    if (!obj_) {
        fprintf(stderr, "failed to open BPF object: %s\n", bpf_obj_path.c_str());
        return false;
    }

    if (bpf_object__load(obj_) < 0) {
        fprintf(stderr, "failed to load BPF object\n");
        bpf_object__close(obj_);
        obj_ = nullptr;
        return false;
    }

    prog_ = bpf_object__find_program_by_name(obj_, "handle_sys_enter");
    if (!prog_) {
        fprintf(stderr, "BPF program not found\n");
        bpf_object__close(obj_);
        obj_ = nullptr;
        return false;
    }

    link_ = bpf_program__attach(prog_);
    if (!link_) {
        fprintf(stderr, "failed to attach BPF program\n");
        bpf_object__close(obj_);
        obj_ = nullptr;
        return false;
    }

    fd_target_cgroup_ = bpf_object__find_map_fd_by_name(obj_, "target_cgroup");
    fd_syscall_count_ = bpf_object__find_map_fd_by_name(obj_, "syscall_count");
    fd_io_bytes_ = bpf_object__find_map_fd_by_name(obj_, "io_bytes");
    fd_net_bytes_ = bpf_object__find_map_fd_by_name(obj_, "net_bytes");

    if (fd_target_cgroup_ < 0 || fd_syscall_count_ < 0 || fd_io_bytes_ < 0 || fd_net_bytes_ < 0) {
        fprintf(stderr, "failed to find BPF maps\n");
        detach();
        return false;
    }

    __u32 key = 0;
    if (bpf_map_update_elem(fd_target_cgroup_, &key, &cgroup_id, BPF_ANY) < 0) {
        fprintf(stderr, "failed to write target cgroup id\n");
        detach();
        return false;
    }

    return true;
}

void EbpfMonitor::detach() {
    if (link_)
        bpf_link__destroy(link_);
    link_ = nullptr;
    if (obj_)
        bpf_object__close(obj_);
    obj_ = nullptr;
    fd_syscall_count_ = -1;
    fd_io_bytes_ = -1;
    fd_net_bytes_ = -1;
    fd_target_cgroup_ = -1;
}

int64_t EbpfMonitor::read_cgroup_u64(const std::string& file) {
    std::ifstream f(file);
    if (!f)
        return 0;
    int64_t val = 0;
    f >> val;
    if (f.fail()) {
        f.clear();
        f.seekg(0);
        std::string line;
        while (std::getline(f, line)) {
            if (line.find("usage_usec") == 0) {
                val = std::stoll(line.substr(11));
                break;
            }
        }
    }
    return val;
}

int64_t EbpfMonitor::lookup_percpu_sum(int fd, unsigned int key) {
    if (fd < 0)
        return 0;

    size_t buf_size = (size_t)nr_cpus_ * 8;
    std::vector<unsigned char> buf(buf_size, 0);

    if (bpf_map_lookup_elem(fd, &key, buf.data()) < 0)
        return 0;

    int64_t sum = 0;
    for (int i = 0; i < nr_cpus_; i++) {
        int64_t val;
        memcpy(&val, buf.data() + i * 8, 8);
        sum += val;
    }
    return sum;
}

EbpfSnapshot EbpfMonitor::poll() {
    EbpfSnapshot snap{};
    snap.job_id = job_id_;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    snap.timestamp = (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;

    __u32 key_read = 0;
    __u32 key_write = 1;

    snap.io_read_bytes = lookup_percpu_sum(fd_io_bytes_, key_read);
    snap.io_write_bytes = lookup_percpu_sum(fd_io_bytes_, key_write);
    snap.net_tx_bytes = lookup_percpu_sum(fd_net_bytes_, key_read);
    snap.net_rx_bytes = lookup_percpu_sum(fd_net_bytes_, key_write);

    __u32 key_read_nr = 0;
    __u32 key_write_nr = 1;
    __u32 key_openat = 257;

    snap.syscall_read_count = lookup_percpu_sum(fd_syscall_count_, key_read_nr);
    snap.syscall_write_count = lookup_percpu_sum(fd_syscall_count_, key_write_nr);
    snap.syscall_openat_count = lookup_percpu_sum(fd_syscall_count_, key_openat);

    snap.cpu_usage_us = read_cgroup_u64(cgroup_path_ + "/cpu.stat");
    snap.mem_current_bytes = read_cgroup_u64(cgroup_path_ + "/memory.current");

    return snap;
}
