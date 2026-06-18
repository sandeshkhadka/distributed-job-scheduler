#include "argparse.hpp"
#include "executors/executor_registry.hpp"
#include "worker_client.hpp"
#include <chrono>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <thread>

using Logger = DJS::Logger;

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("worker");
    program.add_argument("--token", "-k").help("Auth token for the worker (required on first run)");
    program.add_argument("--scheduler", "-s")
        .default_value(std::string("localhost:50051"))
        .help("Scheduler address (default: localhost:50051)");

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

    ExecutorRegistry::init();

    while (true) {
        client.report_pending_results();

        auto job = client.GetJob();
        if (!job) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        Logger::Info("Executing job " + std::to_string(job->job_id) + ": " + job->job_type);

        auto executor = ExecutorRegistry::create(job->job_type);
        if (!executor) {
            client.store_job_result(job->job_id, {false, "unknown job type: " + job->job_type, ""});
            continue;
        }

        auto result = executor->execute(job->params);
        client.store_job_result(job->job_id, result);
        Logger::Info("Job " + std::to_string(job->job_id) + " finished: " + result.message);
    }

    return 0;
}
