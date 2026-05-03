#include "worker_client.hpp"
#include <chrono>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <thread>

using Logger = DJS::Logger;

int main() {
    Logger::Info("Starting worker...");

    // 1. Create a channel to connect to the Scheduler (which runs on port 50051)
    std::shared_ptr<grpc::Channel> channel =
        grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());

    // 2. Initialize the client
    WorkerClient client(channel);

    int port = 50052; // Port where this worker will eventually listen for jobs

    client.Register();

    // ask for jobs every 2 seconds until you get the job
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        client.GetJob();
        const int status = client.execute_job(); // idk what to do with the status
    }

    // Keep the worker alive (optional for now, but needed later when worker acts as a server)
    // while (true) { std::this_thread::sleep_for(std::chrono::seconds(1)); }

    return 0;
}
