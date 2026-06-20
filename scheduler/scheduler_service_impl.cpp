#include "scheduler_service_impl.h"
#include "logger.h"
#include "scheduler_db.h"
#include <grpcpp/support/status.h>
#include <iostream>

using Logger = DJS::Logger;

namespace {

std::string serialize_params(const google::protobuf::Map<std::string, std::string>& params) {
    std::string result;
    for (const auto& [k, v] : params) {
        if (!result.empty())
            result += '\n';
        result += k + "=" + v;
    }
    return result;
}

void deserialize_params(const std::string& encoded,
                        google::protobuf::Map<std::string, std::string>& params) {
    if (encoded.empty())
        return;
    size_t start = 0;
    while (start < encoded.size()) {
        size_t end = encoded.find('\n', start);
        if (end == std::string::npos)
            end = encoded.size();
        auto line = encoded.substr(start, end - start);
        auto eq = line.find('=');
        if (eq != std::string::npos) {
            params[line.substr(0, eq)] = line.substr(eq + 1);
        }
        start = end + 1;
    }
}

} // anonymous namespace

static grpc::Status
check_auth(grpc::ServerContext* context, SchedulerDatabase& db, const std::string& allowed_type) {
    auto md = context->client_metadata().find("authorization");
    if (md == context->client_metadata().end()) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "missing authorization");
    }
    std::string raw(md->second.data(), md->second.length());
    if (raw.substr(0, 7) != "Bearer ") {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "bad authorization format");
    }
    if (!db.is_token_valid(raw.substr(7), allowed_type)) {
        return grpc::Status(grpc::StatusCode::PERMISSION_DENIED, "invalid or revoked token");
    }
    return grpc::Status::OK;
}

grpc::Status SchedulerServiceImpl::SubmitJob(grpc::ServerContext* context,
                                             const djs::SubmitJobRequest* request,
                                             djs::SubmitJobReply* reply) {
    auto auth = check_auth(context, db, "client");
    if (!auth.ok())
        return auth;

    const auto& job_type = request->job_type();
    std::string params_str = serialize_params(request->params());
    int client_id = request->client_id();

    Logger::Info("SubmitJob: type=" + job_type);
    int id = db.insert_job(job_type, params_str, client_id);
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
    auto auth = check_auth(context, db, "worker");
    if (!auth.ok())
        return auth;
    Worker worker;
    worker.cpu_cores = request->cpu_cores();
    worker.mem_size = request->mem_size();
    worker.disk_size = request->disk_size();
    worker.cpu_freq = request->cpu_freq();
    worker.os = request->os();
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
    auto auth = check_auth(context, db, "worker");
    if (!auth.ok())
        return auth;
    const int worker_id = request->worker_id();

    if (worker_id <= 0) {
        return grpc::Status(grpc::INVALID_ARGUMENT, "worker_id is required");
    }
    Worker worker = db.get_worker_by_id(worker_id);
    if (worker.id == 0) {
        return grpc::Status(grpc::NOT_FOUND, "Worker not found");
    }

    std::vector<Job> jobs = db.get_jobs_by_status("not started");
    Job selected_job = job_selector_->select_job(worker, jobs);

    if (selected_job.id == 0) {
        return grpc::Status(grpc::NOT_FOUND, "No jobs available");
    }

    db.insert_worker_job({worker_id, selected_job.id});
    db.update_job_status(selected_job.id, "ongoing");

    reply->set_job_id(selected_job.id);
    reply->set_job_type(selected_job.job_type);
    deserialize_params(selected_job.params, *reply->mutable_params());
    return grpc::Status::OK;
}

// returns the client id
grpc::Status SchedulerServiceImpl::RegisterClient(grpc::ServerContext* context,
                                                  const djs::RegisterClientRequest* request,
                                                  djs::RegisterClientResponse* response) {
    auto auth = check_auth(context, db, "client");
    if (!auth.ok())
        return auth;

    Client client;
    client.name = request->hostname();
    client.status = "active";
    int client_id = db.insert_client(client);

    auto md = context->client_metadata().find("authorization");
    std::string raw(md->second.data(), md->second.length());
    int token_id = db.get_token_id(raw.substr(7));
    if (token_id > 0) {
        db.record_client_token_usage(client_id, token_id);
    }

    std::cout << "Client Registered: [" << client_id << "]\n";

    response->set_ok(true);
    response->set_message("Client successfully registered");
    response->set_client_id(client_id);
    return grpc::Status::OK;
}

grpc::Status SchedulerServiceImpl::ReportJobResult(grpc::ServerContext* context,
                                                   const djs::ReportJobResultRequest* request,
                                                   djs::ReportJobResultReply* reply) {
    auto auth = check_auth(context, db, "worker");
    if (!auth.ok())
        return auth;

    int job_id = request->job_id();
    bool success = request->success();
    std::string message = request->success() ? "completed" : "failed: " + request->message();
    std::string artifact_url = request->artifact_url();

    std::string status = request->success() ? "completed" : "failed";
    db.update_job_status(job_id, status);
    db.save_job_result(job_id, success, message, artifact_url);

    Logger::Info("Job " + std::to_string(job_id) + " " + status);

    reply->set_ok(true);
    return grpc::Status::OK;
}

grpc::Status SchedulerServiceImpl::ConfirmJobReceived(grpc::ServerContext* context,
                                                      const djs::ConfirmJobReceivedRequest* request,
                                                      djs::ConfirmJobReceivedResponse* reply) {
    auto auth = check_auth(context, db, "worker");
    if (!auth.ok())
        return auth;

    int job_id = request->job_id();
    db.update_job_status(job_id, "scheduled");
    Logger::Info("Job " + std::to_string(job_id) + " confirmed by worker " +
                 std::to_string(request->worker_id()));

    reply->set_accepted(true);
    reply->set_message("confirmed");
    return grpc::Status::OK;
}

grpc::Status SchedulerServiceImpl::ReportJobStarted(grpc::ServerContext* context,
                                                    const djs::ReportJobStartedRequest* request,
                                                    djs::ReportJobStartedResponse* reply) {
    auto auth = check_auth(context, db, "worker");
    if (!auth.ok())
        return auth;

    int job_id = request->job_id();
    db.update_job_status(job_id, "started");
    Logger::Info("Job " + std::to_string(job_id) + " started by worker " +
                 std::to_string(request->worker_id()));

    reply->set_accepted(true);
    reply->set_message("started");
    return grpc::Status::OK;
}

grpc::Status SchedulerServiceImpl::ReportWorkerMetrics(grpc::ServerContext* context,
                                                       const djs::WorkerMetrics* request,
                                                       djs::ReportWorkerMetricsResponse* reply) {
    auto auth = check_auth(context, db, "worker");
    if (!auth.ok())
        return auth;

    db.save_worker_metrics(request->worker_id(),
                           request->cpu_percent(),
                           request->memory_percent(),
                           request->memory_used_mb(),
                           request->memory_total_mb(),
                           request->disk_used_mb(),
                           request->disk_total_mb(),
                           request->disk_percent(),
                           request->rx_bytes_per_sec(),
                           request->tx_bytes_per_sec(),
                           request->load_avg_1m(),
                           request->active_jobs());

    Logger::Info("Worker " + std::to_string(request->worker_id()) +
                 " metrics: cpu=" + std::to_string(request->cpu_percent()) +
                 "% mem=" + std::to_string(request->memory_percent()) + "%");

    reply->set_accepted(true);
    return grpc::Status::OK;
}
