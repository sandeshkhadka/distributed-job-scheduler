#include "argparse.hpp"
#include "job_orchestrator.hpp"
#include "worker_client.hpp"
#include <chrono>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <thread>

using Logger = DJS::Logger;

namespace {

std::string serialize_params(const std::map<std::string, std::string>& params) {
    std::string result;
    for (const auto& [k, v] : params) {
        if (!result.empty())
            result += '\n';
        result += k + "=" + v;
    }
    return result;
}

void recover_pending_jobs(WorkerClient& client, WorkerDatabase& db) {
    auto pending = db.get_pending_jobs();
    if (pending.empty())
        return;

    Logger::Info("Recovering " + std::to_string(pending.size()) + " pending job(s) from crash...");
    for (const auto& rec : pending) {
        Logger::Info("Job " + std::to_string(rec.job_id) + " was " + rec.status +
                     " before crash. Reporting as failed.");
        client.store_job_result(rec.job_id, {false, "failed: worker restarted", ""});
        db.update_received_job_status(rec.job_id, "failed");
    }
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("worker");
    program.add_argument("--token", "-k").help("Auth token for the worker (required on first run)");
    program.add_argument("--scheduler", "-s")
        .default_value(std::string("localhost:50051"))
        .help("Scheduler address (default: localhost:50051)");
    program.add_argument("--executor", "-e")
        .default_value(std::string("djs-executor"))
        .help("Path to djs-executor binary (default: djs-executor)");

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    std::string scheduler_addr = program.get<std::string>("--scheduler");
    Logger::Info("Starting worker...");
    Logger::Info("Connecting to scheduler at " + scheduler_addr);

    std::shared_ptr<grpc::Channel> channel =
        grpc::CreateChannel(scheduler_addr, grpc::InsecureChannelCredentials());

    WorkerClient client(channel);

    std::string token;
    try {
        token = program.get<std::string>("--token");
    } catch (const std::logic_error&) {
    }

    int registered = -1;
    if (!token.empty()) {
        registered = client.Register(token);
    } else {
        registered = client.Register();
    }

    if (registered < 0) {
        Logger::Error("Registration failed. Exiting.");
        return 1;
    }

    recover_pending_jobs(client, client.db);

    std::string executor_path = program.get<std::string>("--executor");
    if (executor_path == "djs-executor") {
        std::string self = argv[0];
        auto slash = self.rfind('/');
        if (slash != std::string::npos) {
            executor_path = self.substr(0, slash + 1) + "djs-executor";
        }
    }

    JobOrchestrator orchestrator;
    orchestrator.executor_path = executor_path;
    orchestrator.on_completed = [&](int job_id, const JobResult& result) {
        client.store_job_result(job_id, result);
        client.db.update_received_job_status(job_id, result.success ? "completed" : "failed");
        Logger::Info("Orchestrator completed job " + std::to_string(job_id) + ": " +
                     result.message);
    };

    while (true) {
        client.report_pending_results();

        auto job = client.GetJob();
        if (!job) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        client.db.store_received_job(job->job_id, job->job_type, serialize_params(job->params));
        Logger::Info("Stored job " + std::to_string(job->job_id) + " locally before confirmation");

        if (!client.confirm_job_received(job->job_id)) {
            client.store_job_result(job->job_id,
                                    {false, "failed: confirm job received failed", ""});
            client.db.update_received_job_status(job->job_id, "failed");
            continue;
        }
        client.db.update_received_job_status(job->job_id, "scheduled");

        if (!client.report_job_started(job->job_id)) {
            client.store_job_result(job->job_id, {false, "failed: report job started failed", ""});
            client.db.update_received_job_status(job->job_id, "failed");
            continue;
        }
        client.db.update_received_job_status(job->job_id, "started");

        Logger::Info("Launching job " + std::to_string(job->job_id) + ": " + job->job_type);

        auto handle = orchestrator.execute(job->job_id, job->job_type, job->params);
        if (handle.pid < 0) {
            client.store_job_result(job->job_id, {false, "failed: orchestration setup failed", ""});
            client.db.update_received_job_status(job->job_id, "failed");
            continue;
        }

        Logger::Info("Job " + std::to_string(job->job_id) + " running as PID " +
                     std::to_string(handle.pid));
    }

    return 0;
}
