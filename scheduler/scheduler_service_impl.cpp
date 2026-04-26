#include "scheduler_service_impl.h"
#include <iostream>

grpc::Status SchedulerServiceImpl::SubmitJob(grpc::ServerContext* context,
                                             const djs::SubmitJobRequest* request,
                                             djs::SubmitJobReply* reply) {

    const std::string& payload = request->payload();

    std::cout << "Message: " << payload << " aayo haita" << std::endl;
    std::string job_id = "random_job_id";

    jobs[job_id] = "queued";

    reply->set_accepted(true);
    reply->set_message("job accepted");
    reply->set_job_id(job_id);

    return grpc::Status::OK;
};

grpc::Status SchedulerServiceImpl::RegisterWorker(grpc::ServerContext* context,
                                                  const djs::RegisterWorkerRequest* request,
                                                  djs::RegisterWorkerReply* reply) {
    const std::string& worker_id = request->worker_id();
    const std::string& host = request->host();
    int32_t port = request->port();

    if (worker_id.empty() || host.empty() || port <= 0) {
        reply->set_ok(false);
        reply->set_message("Invalid worker registration details");
        return grpc::Status::OK;
    }

    // Save the worker in our state
    std::string address = host + ":" + std::to_string(port);
    active_workers[worker_id] = address;

    std::cout << "Worker Registered: [" << worker_id << "] at " << address << std::endl;

    reply->set_ok(true);
    reply->set_message("Worker successfully registered");

    return grpc::Status::OK;
}
