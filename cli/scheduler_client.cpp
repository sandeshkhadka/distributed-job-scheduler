#include "scheduler_client.h"

#include <iostream>

SchedulerClient::SchedulerClient(std::shared_ptr<grpc::Channel> channel)
    : stub(djs::SchedulerService::NewStub(channel)) {}

void SchedulerClient::SubmitJob(const std::string& payload) {

    int client_id = db.get_active_client();
    if (client_id <= 0) {
        std::cout << "Error: no registered client found. Please run 'register' first.\n";
        return;
    }

    djs::SubmitJobRequest request;
    request.set_client_id(client_id);
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

void SchedulerClient::RegisterClient(const std::string& hostname) {

    djs::RegisterClientRequest request;
    request.set_hostname(hostname);

    djs::RegisterClientResponse reply;
    grpc::ClientContext context;

    grpc::Status status = stub->RegisterClient(&context, request, &reply);

    if (status.ok()) {
        if (reply.ok()) {
            db.insert_client(hostname, reply.client_id());
            std::cout << "message: " << reply.message() << "\n";
            std::cout << "client_id: " << reply.client_id() << "\n";
        } else {
            std::cout << "registration failed: " << reply.message() << "\n";
        }
    } else {
        std::cout << "RPC failed: " << status.error_message() << "\n";
    }
}
