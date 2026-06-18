#include "worker_client.hpp"

WorkerClient::WorkerClient(std::shared_ptr<grpc::Channel> channel)
    : stub_(djs::SchedulerService::NewStub(channel)) {
    token_ = db.get_token();
}

void WorkerClient::add_auth(grpc::ClientContext& context) {
    if (!token_.empty()) {
        context.AddMetadata("authorization", "Bearer " + token_);
    }
}

int WorkerClient::Register() {
    if (this->worker_id != -1) {
        return this->worker_id;
    }

    if (int cached = db.get_registered_worker_id(); cached != -1) {
        Logger::Info("Registration: Worker already registered.");
        this->worker_id = cached;
        return cached;
    }

    if (token_.empty()) {
        Logger::Error(
            "No auth token available. Run the worker with --token <auth_token> to authenticate.");
        return -1;
    }

    return do_register();
}

int WorkerClient::Register(const std::string& token) {
    token_ = token;
    this->worker_id = -1;
    return do_register();
}

int WorkerClient::do_register() {
    Logger::Info("Registration: Registering worker...");

    const std::string WORKER_NAME = "UniqueName";

    djs::RegisterWorkerRequest request;
    request.set_cpu_cores(5);
    request.set_mem_size(16);
    request.set_disk_size(500);
    request.set_name(WORKER_NAME);
    request.set_cpu_freq(3.5);
    request.set_os("Linux");

    djs::RegisterWorkerReply reply;
    grpc::ClientContext context;
    add_auth(context);

    grpc::Status status = stub_->RegisterWorker(&context, request, &reply);

    if (status.ok()) {
        if (reply.ok()) {
            this->worker_id = reply.worker_id();
            db.insert_worker(Worker{this->worker_id, WORKER_NAME});
            db.save_token(token_);
            Logger::Info("Registration: successful: Id: " + std::to_string(worker_id));
        } else {
            Logger::Info("Registration: rejected: " + reply.message());
        }
    } else {
        Logger::Error("Registration failed: " + status.error_message());
        if (status.error_code() == grpc::StatusCode::UNAUTHENTICATED ||
            status.error_code() == grpc::StatusCode::PERMISSION_DENIED) {
            Logger::Error("Run the worker with --token <auth_token> to authenticate.");
        }
    }
    return worker_id;
}

void WorkerClient::GetJob() {
    djs::GetJobRequest request;
    djs::GetJobResponse reply;
    grpc::ClientContext context;

    if (this->worker_id == -1) {
        Logger::Error("This worker is not registered. Register before running jobs!");
        exit(0);
    }

    request.set_worker_id(this->worker_id);
    add_auth(context);

    grpc::Status status = stub_->GetJob(&context, request, &reply);
    if (status.ok()) {
        std::cout << "Job ID: " << reply.job_id() << std::endl;
        std::cout << "Type: " << reply.job_type() << std::endl;
    } else {
        Logger::Info("RPC failed: " + status.error_message());
    }
}
