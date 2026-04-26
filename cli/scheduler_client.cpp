#include "scheduler_client.h"

#include <iostream>

SchedulerClient::SchedulerClient(std::shared_ptr<grpc::Channel> channel)
    : stub(djs::SchedulerService::NewStub(channel)) {}

void SchedulerClient::SubmitJob(const std::string& payload) {

    djs::SubmitJobRequest request;
    request.set_payload(payload);

    djs::SubmitJobReply reply;

    grpc::ClientContext context;

    grpc::Status status = stub->SubmitJob(&context, request, &reply);

    if (status.ok()) {
        std::cout << "accepted: " << reply.accepted() << "\n";
        std::cout << "message: " << reply.message() << "\n";
        std::cout << "jobid: " << reply.job_id() << "\n";
    } else {
        std::cout << "RPC failed: " << status.error_message() << "\n";
    }
}
