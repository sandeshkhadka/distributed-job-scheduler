#include "./scheduler_service_impl.h"
#include "logger.h"
#include <grpcpp/grpcpp.h>

using Logger = DJS::Logger;

int main() {
    SchedulerServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    server->Wait();
    return 0;
}
