#include "./scheduler_service_impl.h"
#include "job_selector_registry.hpp"
#include "logger.h"
#include "scheduler_db.h"
#include <grpcpp/grpcpp.h>

using Logger = DJS::Logger;

int main() {
    JobSelectorRegistry registry;
    registry.init_all();
    std::string algo = "adaptive";

    SchedulerServiceImpl service;
    service.job_selector_ = registry.create(algo);
    if (!service.job_selector_) {
        Logger::Error("Unknown job selection algorithm: " + algo);
        return 1;
    }
    Logger::Info("Using job selector: " + service.job_selector_->name());

    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    Logger::Info("Server started on port 50051");
    server->Wait();
    return 0;
}
