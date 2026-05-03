#include "scheduler_service_impl.h"
#include "logger.h"
#include "scheduler_db.h"
#include <grpcpp/support/status.h>
#include <iostream>

using Logger = DJS::Logger;

grpc::Status SchedulerServiceImpl::SubmitJob(grpc::ServerContext* context,
                                             const djs::SubmitJobRequest* request,
                                             djs::SubmitJobReply* reply) {

    const std::string& payload = request->payload();

    int client_id = request->client_id();

    Logger::Info("SubmitJob: payload=" + payload);
    int id = db.insert_job(payload, client_id);
    if (id > 0) {
        std::string job_id = std::to_string(id);
        reply->set_accepted(true);
        reply->set_message("job accepted");
        reply->set_job_id(job_id);
        Logger::Info("Job created: job_id=" + job_id);
        return grpc::Status::OK;
    }

    std::string job_id = "-1";
    reply->set_accepted(false);
    reply->set_message("job rejected");
    reply->set_job_id(job_id);

    return grpc::Status::OK;
};

grpc::Status SchedulerServiceImpl::RegisterWorker(grpc::ServerContext* context,
                                                  const djs::RegisterWorkerRequest* request,
                                                  djs::RegisterWorkerReply* reply) {

    Worker worker;
    worker.cpu_cores = request->cpu_cores();
    worker.mem_size = request->mem_size();
    worker.disk_size = request->disk_size();
    worker.cpu_freq = request->cpu_freq();
    worker.os = request->os();
    worker.kernel_version = request->kernel_version();
    worker.name = request->name();

    int worker_id = db.insert_worker(worker);

    std::cout << "Worker Registered: [" << worker_id << "]\n";

    reply->set_ok(true);
    reply->set_message("Worker successfully registered");
    reply->set_worker_id(worker_id);
    return grpc::Status::OK;
}

grpc::Status SchedulerServiceImpl::GetJob(grpc::ServerContext* context,
                                          const djs::GetJobRequest* request,
                                          djs::GetJobResponse* reply) {
    const int worker_id = request->worker_id();

    if (worker_id <= 0) {
        return grpc::Status(grpc::INVALID_ARGUMENT, "worker_id is required");
    }
    Worker worker = db.get_worker_by_id(worker_id);
    if (worker.id == 0) {
        return grpc::Status(grpc::NOT_FOUND, "Worker not found");
    }

    std::vector<Job> jobs;
    jobs = db.get_worker_jobs(worker_id);
    Job selected_job = select_job(worker, jobs);

    if (selected_job.id == 0) {
        jobs = db.get_jobs_by_status("not started");
        selected_job = select_job(worker, jobs);
    }

    if (selected_job.id == 0) {
        return grpc::Status(grpc::NOT_FOUND, "No jobs available");
    }

    reply->set_job_id(selected_job.id);
    reply->set_payload(selected_job.payload);
    return grpc::Status::OK;
}

// returns the client id
grpc::Status SchedulerServiceImpl::RegisterClient(grpc::ServerContext* context,
                                                  const djs::RegisterClientRequest* request,
                                                  djs::RegisterClientResponse* response) {
    Client client;
    client.name = request->hostname();
    client.status = "active";
    int client_id = db.insert_client(client);

    std::cout << "Client Registered: [" << client_id << "]\n";

    response->set_ok(true);
    response->set_message("Client successfully registered");
    response->set_client_id(client_id);
    return grpc::Status::OK;
}

Job SchedulerServiceImpl::select_job(const Worker& worker, const std::vector<Job>& jobs) {
    for (auto job : jobs) {
        if (job.status == "not started") {
            return job;
        }
    }
    return Job{};
}
