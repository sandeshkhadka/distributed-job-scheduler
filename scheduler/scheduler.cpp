#include "./scheduler_service_impl.h"
#include "logger.h"
#include "scheduler_db.h"
#include <grpcpp/grpcpp.h>

using Logger = DJS::Logger;

int main() {
    SchedulerServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    Logger::Info("Server started on port 50051");
    server->Wait();
    return 0;
}
