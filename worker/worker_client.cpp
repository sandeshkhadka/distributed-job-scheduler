#include "worker_client.hpp"

WorkerClient::WorkerClient(std::shared_ptr<grpc::Channel> channel)
    : stub_(djs::SchedulerService::NewStub(channel)) {}

void WorkerClient::Register(const std::string& worker_id, const std::string& host, int port) {
    djs::RegisterWorkerRequest request;
    request.set_worker_id(worker_id);
    request.set_host(host);
    request.set_port(port);

    djs::RegisterWorkerReply reply;
    grpc::ClientContext context;

    // Perform the RPC call
    grpc::Status status = stub_->RegisterWorker(&context, request, &reply);

    if (status.ok()) {
        if (reply.ok()) {
            Logger::Info("Registration successful: " + reply.message());
        } else {
            Logger::Info("Registration rejected: " + reply.message());
        }
    } else {
        Logger::Info("RPC failed: " + status.error_message());
    }
}