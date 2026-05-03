#include "worker_client.hpp"

WorkerClient::WorkerClient(std::shared_ptr<grpc::Channel> channel)
    : stub_(djs::SchedulerService::NewStub(channel)) {}

int WorkerClient::Register() {
    // worker is already registered
    if (this->worker_id != -1) {
        return this->worker_id;
    }

    if (int registered_worker_id = db.get_registered_worker_id(); registered_worker_id != -1) {
        Logger::Info("Registration: Worker already registered.");
        this->worker_id = registered_worker_id;
        return registered_worker_id;
    }

    Logger::Info("Registration: Worker not registered. Registering worker...");
    djs::RegisterWorkerRequest request;

    const int CPU_CORES = 5;
    const float MEM_SIZE = 5;
    const float DISK_SIZE = 500;
    const float CPU_FREQ = 3.5;
    const std::string OS = "Linux";
    const std::string WORKER_NAME = "UniqueName";
    const std::string STATUS = "not running";

    request.set_cpu_cores(CPU_CORES);
    request.set_mem_size(MEM_SIZE);
    request.set_disk_size(DISK_SIZE);
    request.set_name(WORKER_NAME);
    request.set_cpu_freq(CPU_FREQ);
    request.set_os(OS);
    djs::RegisterWorkerReply reply;
    grpc::ClientContext context;

    // Perform the RPC call
    grpc::Status status = stub_->RegisterWorker(&context, request, &reply);
    // int worker_id{0};
    if (status.ok()) {
        if (reply.ok()) {
            this->worker_id = reply.worker_id();
            db.insert_worker(Worker{this->worker_id,
                                    CPU_CORES,
                                    MEM_SIZE,
                                    DISK_SIZE,
                                    CPU_FREQ,
                                    OS,
                                    WORKER_NAME,
                                    STATUS});

            Logger::Info("Registration: successful: Id: " + std::to_string(worker_id));
        } else {
            Logger::Info("Registration: rejected: " + reply.message());
        }
    } else {
        Logger::Info("RPC failed: " + status.error_message());
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
    // Perform the RPC call
    grpc::Status status = stub_->GetJob(&context, request, &reply);
    if (status.ok()) {
        std::cout << "Job ID: " << reply.job_id() << std::endl;
        std::cout << "Payload: " << reply.payload() << std::endl;
        std::system(reply.payload().c_str());
    } else {
        Logger::Info("RPC failed: " + status.error_message());
    }
}
