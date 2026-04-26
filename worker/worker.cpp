#include "worker_client.hpp"
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>

using Logger = DJS::Logger;

int main() {
    Logger::Info("Starting worker...");

    // 1. Create a channel to connect to the Scheduler (which runs on port 50051)
    std::shared_ptr<grpc::Channel> channel =
        grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());

    // 2. Initialize the client
    WorkerClient client(channel);

    // 3. Register this worker with the scheduler
    // In a real system, you might generate a UUID and use the machine's actual IP
    std::string worker_id = "worker-node-1";
    std::string host = "127.0.0.1";
    int port = 50052; // Port where this worker will eventually listen for jobs

    client.Register(worker_id, host, port);

    // Keep the worker alive (optional for now, but needed later when worker acts as a server)
    // while (true) { std::this_thread::sleep_for(std::chrono::seconds(1)); }

    return 0;
}
