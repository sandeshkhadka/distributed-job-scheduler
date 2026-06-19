#pragma once
#include "executors/job_executor.hpp"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

struct JobHandle {
    int job_id;
    int pid;
    std::string cgroup_path;
    int result_fd;
};

class JobOrchestrator {
  public:
    std::function<void(int job_id, const JobResult& result)> on_completed;

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
    std::string make_cgroup(int job_id);
    void remove_cgroup(const std::string& path);
    JobResult parse_result(const std::string& json);
    std::vector<std::string> build_exec_args(int job_id,
                                             const std::string& job_type,
                                             const std::map<std::string, std::string>& params);
};
