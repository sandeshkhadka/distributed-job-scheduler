#include "scheduler_service_impl.h"
#include <iostream>

grpc::Status SchedulerServiceImpl::SubmitJob(grpc::ServerContext* context,
                                             const djs::SubmitJobRequest* request,
                                             djs::SubmitJobReply* reply) {

    const std::string& job_id = request->job_id();
    const std::string& payload = request->payload();

    if (job_id.empty()) {
        reply->set_accepted(false);
        reply->set_message("job_id is empty");
        return grpc::Status::OK;
    }
    std::cout << "Job: " << job_id << " aayo haita" << std::endl;
    std::cout << "Message: " << payload << " aayo haita" << std::endl;

    jobs[job_id] = "queued";

    reply->set_accepted(true);
    reply->set_message("job accepted");

    return grpc::Status::OK;
};
