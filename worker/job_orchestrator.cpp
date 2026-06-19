#include "job_orchestrator.hpp"
#include "logger.h"
#include <cerrno>
#include <chrono>
#include <fstream>
#include <sstream>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

using Logger = DJS::Logger;

JobOrchestrator::JobOrchestrator() : running_(true) {
    monitor_ = std::thread(&JobOrchestrator::monitor_loop, this);
}

JobOrchestrator::~JobOrchestrator() {
    running_ = false;
    if (monitor_.joinable())
        monitor_.join();
}

std::string JobOrchestrator::make_cgroup(int job_id) {
    std::string path = "/sys/fs/cgroup/djs/jobs/" + std::to_string(job_id);
    std::string cmd = "mkdir -p " + path;
    if (system(cmd.c_str()) != 0) {
        Logger::Error("Failed to create cgroup: " + path);
    }
    return path;
}

void JobOrchestrator::remove_cgroup(const std::string& path) {
    if (path.empty())
        return;
    std::string cmd = "rmdir " + path;
    system(cmd.c_str());
}

std::vector<std::string> JobOrchestrator::build_exec_args(
    int job_id, const std::string& job_type, const std::map<std::string, std::string>& params) {
    (void)job_id;
    std::vector<std::string> args;
    args.emplace_back(executor_path);
    args.emplace_back("--type");
    args.emplace_back(job_type);
    for (const auto& [k, v] : params) {
        args.emplace_back("--param");
        args.emplace_back(k + "=" + v);
    }
    return args;
}

JobResult JobOrchestrator::parse_result(const std::string& json) {
    JobResult result{false, "", ""};
    result.success = json.find("\"success\":true") != std::string::npos;
    auto msg_start = json.find("\"message\":\"");
    if (msg_start != std::string::npos) {
        msg_start += 11;
        auto msg_end = json.find("\"", msg_start);
        if (msg_end != std::string::npos)
            result.message = json.substr(msg_start, msg_end - msg_start);
    }
    auto url_start = json.find("\"artifact_url\":\"");
    if (url_start != std::string::npos) {
        url_start += 16;
        auto url_end = json.find("\"", url_start);
        if (url_end != std::string::npos)
            result.artifact_url = json.substr(url_start, url_end - url_start);
    }
    return result;
}

JobHandle JobOrchestrator::execute(int job_id,
                                   const std::string& job_type,
                                   const std::map<std::string, std::string>& params) {
    std::string cg_path = make_cgroup(job_id);

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        Logger::Error("pipe failed for job " + std::to_string(job_id));
        return {job_id, -1, "", -1};
    }

    auto args = build_exec_args(job_id, job_type, params);
    std::vector<const char*> cargs;
    for (const auto& a : args)
        cargs.push_back(a.c_str());
    cargs.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        unshare(CLONE_NEWPID | CLONE_NEWNS);

        std::string cg_procs = cg_path + "/cgroup.procs";
        std::ofstream cg(cg_procs);
        if (cg.is_open())
            cg << getpid();
        cg.close();

        execvp(executor_path.c_str(), const_cast<char* const*>(cargs.data()));
        _exit(1);
    }

    close(pipefd[1]);

    JobHandle handle{job_id, static_cast<int>(pid), cg_path, pipefd[0]};

    std::string cg_procs = cg_path + "/cgroup.procs";
    std::ofstream cg(cg_procs);
    if (cg.is_open())
        cg << pid;
    cg.close();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        active_jobs_[job_id] = handle;
    }

    Logger::Info("Launched job " + std::to_string(job_id) + " as PID " + std::to_string(pid));
    return handle;
}

void JobOrchestrator::monitor_loop() {
    while (running_) {
        int status;
        pid_t pid = waitpid(-1, &status, 0);
        if (pid <= 0) {
            if (!running_)
                break;
            if (errno == ECHILD) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }

        int job_id = -1;
        int result_fd = -1;
        std::string cg_path;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& [id, h] : active_jobs_) {
                if (h.pid == static_cast<int>(pid)) {
                    job_id = id;
                    result_fd = h.result_fd;
                    cg_path = h.cgroup_path;
                    break;
                }
            }
        }

        if (job_id < 0)
            continue;

        std::string json;
        char buf[512];
        ssize_t n;
        while ((n = read(result_fd, buf, sizeof(buf) - 1)) > 0) {
            buf[n] = '\0';
            json += buf;
        }
        close(result_fd);

        auto result = parse_result(json);
        bool exited_ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
        if (!exited_ok) {
            result.success = false;
            if (result.message.empty())
                result.message = "failed: exit status " + std::to_string(WEXITSTATUS(status));
        }

        teardown_job(job_id, result);
    }
}

void JobOrchestrator::teardown_job(int job_id, const JobResult& result) {
    std::string cg_path;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = active_jobs_.find(job_id);
        if (it != active_jobs_.end()) {
            cg_path = it->second.cgroup_path;
            active_jobs_.erase(it);
        }
    }

    if (!cg_path.empty())
        remove_cgroup(cg_path);

    Logger::Info("Job " + std::to_string(job_id) + " finished: " + result.message);

    if (on_completed)
        on_completed(job_id, result);
}
