#include "job_orchestrator.hpp"
#include "logger.h"
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
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
    system(cmd.c_str());
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

static int fork_monitor(const std::string& path,
                        int job_id,
                        int target_pid,
                        const std::string& cgroup_path,
                        int out_pipe[2]) {
    if (pipe(out_pipe) < 0)
        return -1;

    pid_t pid = fork();
    if (pid == 0) {
        close(out_pipe[0]);
        dup2(out_pipe[1], STDOUT_FILENO);
        close(out_pipe[1]);

        std::string jid = std::to_string(job_id);
        std::string pid_s = std::to_string(target_pid);

        execlp(path.c_str(),
               path.c_str(),
               "--job-id",
               jid.c_str(),
               "--pid",
               pid_s.c_str(),
               "--cgroup-path",
               cgroup_path.c_str(),
               nullptr);
        _exit(2);
    }

    close(out_pipe[1]);
    return out_pipe[0];
}

JobHandle JobOrchestrator::execute(int job_id,
                                   const std::string& job_type,
                                   const std::map<std::string, std::string>& params) {
    std::string cg_path = make_cgroup(job_id);

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        Logger::Error("pipe failed for job " + std::to_string(job_id));
        return {job_id, -1, "", -1, -1, std::thread()};
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

        try {
            execvp(executor_path.c_str(), const_cast<char* const*>(cargs.data()));
        } catch (const std::exception& e) {
            std::string msg = "{\"success\":false,\"message\":\"failed: ";
            msg += e.what();
            msg += "\"}";
            write(STDOUT_FILENO, msg.data(), msg.size());
            _exit(1);
        }
        std::string msg = "{\"success\":false,\"message\":\"failed: execvp: ";
        msg += strerror(errno);
        msg += "\"}";
        write(STDOUT_FILENO, msg.data(), msg.size());
        _exit(1);
    }

    close(pipefd[1]);

    int ebpf_fd = -1;
    if (!ebpf_monitor_path.empty()) {
        int ebpf_pipe[2];
        ebpf_fd =
            fork_monitor(ebpf_monitor_path, job_id, static_cast<int>(pid), cg_path, ebpf_pipe);
        if (ebpf_fd < 0) {
            Logger::Info("ebpf monitor not available for job " + std::to_string(job_id));
        }
    }

    JobHandle handle{job_id, static_cast<int>(pid), cg_path, pipefd[0], ebpf_fd, std::thread()};

    std::string cg_procs = cg_path + "/cgroup.procs";
    std::ofstream cg(cg_procs);
    if (cg.is_open())
        cg << pid;
    cg.close();

    if (ebpf_fd >= 0 && metric_store) {
        handle.ebpf_reader =
            std::thread(&JobOrchestrator::ebpf_reader_thread, this, job_id, ebpf_fd);
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        active_jobs_[job_id] = std::move(handle);
    }

    Logger::Info("Launched job " + std::to_string(job_id) + " as PID " + std::to_string(pid));
    return handle;
}

djs::JobEbpfMetrics JobOrchestrator::parse_ebpf_json(const std::string& line) {
    djs::JobEbpfMetrics m;
    m.set_job_id(0);
    m.set_timestamp(0);

    auto extract = [&](const std::string& key) -> std::string {
        auto pos = line.find("\"" + key + "\":");
        if (pos == std::string::npos)
            return "";
        pos += key.size() + 3;
        auto end = line.find_first_of(",\n}", pos);
        if (end == std::string::npos)
            return "";
        return line.substr(pos, end - pos);
    };

    auto id_str = extract("job_id");
    if (!id_str.empty())
        m.set_job_id(std::stoll(id_str));

    auto ts_str = extract("ts");
    if (!ts_str.empty())
        m.set_timestamp(std::stod(ts_str));

    auto sr = extract("syscall_read");
    if (!sr.empty())
        m.set_syscall_read_count(std::stoll(sr));
    auto sw = extract("syscall_write");
    if (!sw.empty())
        m.set_syscall_write_count(std::stoll(sw));
    auto so = extract("syscall_openat");
    if (!so.empty())
        m.set_syscall_openat_count(std::stoll(so));

    auto io_r = extract("io_read_bytes");
    if (!io_r.empty())
        m.set_io_read_bytes(std::stoll(io_r));
    auto io_w = extract("io_write_bytes");
    if (!io_w.empty())
        m.set_io_write_bytes(std::stoll(io_w));

    auto net_tx = extract("net_tx_bytes");
    if (!net_tx.empty())
        m.set_net_tx_bytes(std::stoll(net_tx));
    auto net_rx = extract("net_rx_bytes");
    if (!net_rx.empty())
        m.set_net_rx_bytes(std::stoll(net_rx));

    auto cpu = extract("cpu_us");
    if (!cpu.empty())
        m.set_cpu_usage_us(std::stoll(cpu));
    auto mem = extract("mem_bytes");
    if (!mem.empty())
        m.set_mem_current_bytes(std::stoll(mem));

    return m;
}

void JobOrchestrator::ebpf_reader_thread(int job_id, int fd) {
    char buf[4096];
    std::string leftover;
    while (true) {
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        if (n <= 0)
            break;
        buf[n] = '\0';
        leftover += buf;

        size_t pos;
        while ((pos = leftover.find('\n')) != std::string::npos) {
            std::string line = leftover.substr(0, pos);
            leftover.erase(0, pos + 1);
            if (line.empty())
                continue;
            auto snap = parse_ebpf_json(line);
            if (snap.job_id() > 0 && metric_store) {
                metric_store->add_snapshot(job_id, snap);
            }
        }
    }
    close(fd);
    Logger::Info("EBPF reader for job " + std::to_string(job_id) + " finished");
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
    std::thread ebpf_reader;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = active_jobs_.find(job_id);
        if (it != active_jobs_.end()) {
            cg_path = it->second.cgroup_path;
            ebpf_reader = std::move(it->second.ebpf_reader);
            active_jobs_.erase(it);
        }
    }

    if (ebpf_reader.joinable())
        ebpf_reader.join();

    if (!cg_path.empty())
        remove_cgroup(cg_path);

    Logger::Info("Job " + std::to_string(job_id) + " finished: " + result.message);

    if (on_completed)
        on_completed(job_id, result);
}
