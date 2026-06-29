#pragma once
#include "ebpf_metric_store.hpp"
#include "executors/job_executor.hpp"
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct JobHandle {
    int job_id;
    int pid;
    std::string cgroup_path;
    int result_fd;
    int ebpf_fd;
    std::thread ebpf_reader;
};

class JobOrchestrator {
  public:
    std::string executor_path = "./build/djs-executor";
    std::string ebpf_monitor_path = "./build/djs-ebpf-monitor";
    std::function<void(int job_id, const JobResult& result)> on_completed;

    EbpfMetricStore* metric_store = nullptr;

    JobOrchestrator();
    ~JobOrchestrator();

    JobHandle execute(int job_id,
                      const std::string& job_type,
                      const std::map<std::string, std::string>& params);

  private:
    std::map<int, JobHandle> active_jobs_;
    std::mutex mutex_;
    std::thread monitor_;
    bool running_;

    void monitor_loop();
    void teardown_job(int job_id, const JobResult& result);
    void ebpf_reader_thread(int job_id, int fd);
    std::string make_cgroup(int job_id);
    void remove_cgroup(const std::string& path);
    JobResult parse_result(const std::string& json);
    std::vector<std::string> build_exec_args(int job_id,
                                             const std::string& job_type,
                                             const std::map<std::string, std::string>& params);
    djs::JobEbpfMetrics parse_ebpf_json(const std::string& line);
};
