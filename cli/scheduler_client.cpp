#include "scheduler_client.h"

#include <iostream>

SchedulerClient::SchedulerClient(std::shared_ptr<grpc::Channel> channel)
    : stub(djs::SchedulerService::NewStub(channel)), cached_client_id_(-1) {
    token_ = db.get_token();
}

void SchedulerClient::add_auth(grpc::ClientContext& context) {
    if (!token_.empty()) {
        context.AddMetadata("authorization", "Bearer " + token_);
    }
}

void SchedulerClient::SubmitJob(const std::string& job_type,
                                const std::map<std::string, std::string>& params) {

    int client_id = db.get_active_client();
    if (client_id <= 0) {
        std::cout << "Error: no registered client found. Please run 'register' first.\n";
        return;
    }

    djs::SubmitJobRequest request;
    request.set_client_id(client_id);
    request.set_job_type(job_type);
    auto* proto_params = request.mutable_params();
    for (const auto& [k, v] : params) {
        (*proto_params)[k] = v;
    }

    djs::SubmitJobReply reply;

    grpc::ClientContext context;
    add_auth(context);

    grpc::Status status = stub->SubmitJob(&context, request, &reply);

    if (status.ok()) {
        std::cout << "accepted: " << reply.accepted() << "\n";
        std::cout << "message: " << reply.message() << "\n";
        std::cout << "jobid: " << reply.job_id() << "\n";
    } else {
        std::cout << "RPC failed: " << status.error_message() << "\n";
    }
}

void SchedulerClient::print_job_result(const CachedJobResultEntry& r, bool from_cache) {
    if (from_cache)
        std::cout << "Cached result (use --refresh to update):\n\n";
    else
        std::cout << "\n";

    std::cout << "Job #" << r.job_id << "\n";
    std::cout << "  Status:       " << r.status << "\n";

    if (r.has_result) {
        std::cout << "  Result:       " << (r.success ? "SUCCESS" : "FAILURE") << "\n";
        std::cout << "  Message:      " << r.result_message << "\n";
        if (!r.artifact_url.empty())
            std::cout << "  Artifact URL: " << r.artifact_url << "\n";
        std::cout << "  Completed at: " << r.completed_at << "\n";
    }

    if (from_cache && !r.cached_at.empty())
        std::cout << "  (cached at: " << r.cached_at << ")\n";
}

int SchedulerClient::get_client_id() {
    if (cached_client_id_ > 0)
        return cached_client_id_;
    cached_client_id_ = db.get_active_client();
    return cached_client_id_;
}

void SchedulerClient::ListMyJobs(bool refresh) {
    int client_id = get_client_id();
    if (client_id <= 0) {
        std::cout << "Error: no registered client. Please run 'register' first.\n";
        return;
    }

    std::vector<CachedJobEntry> jobs;

    if (!refresh) {
        jobs = db.get_cached_jobs();
        if (!jobs.empty()) {
            std::cout << "Cached jobs (use --refresh to update):\n";
        }
    }

    if (refresh || jobs.empty()) {
        djs::GetClientJobsRequest request;
        request.set_client_id(client_id);

        djs::GetClientJobsResponse reply;
        grpc::ClientContext context;
        add_auth(context);

        grpc::Status status = stub->GetClientJobs(&context, request, &reply);

        if (!status.ok()) {
            std::cout << "RPC failed: " << status.error_message() << "\n";
            return;
        }

        std::vector<CachedJobEntry> fetched;
        for (const auto& j : reply.jobs()) {
            fetched.push_back({j.job_id(), j.job_type(), j.status(), j.created_at(), ""});
        }
        db.cache_jobs(fetched);
        jobs = fetched;
    }

    if (jobs.empty()) {
        std::cout << "No jobs found.\n";
        return;
    }

    std::cout << "Job ID  |  Type        |  Status        |  Created At\n";
    std::cout << "--------|--------------|----------------|-----------------------------\n";
    for (const auto& j : jobs) {
        printf("%-7d | %-12s | %-14s | %s\n",
               j.job_id,
               j.job_type.c_str(),
               j.status.c_str(),
               j.created_at.c_str());
    }
}

void SchedulerClient::GetJobResult(int job_id, bool refresh) {
    if (!refresh) {
        auto cached = db.get_cached_job_result(job_id);
        if (cached.job_id != 0) {
            print_job_result(cached, true);
            return;
        }
    }

    djs::GetJobStatusRequest request;
    request.set_job_id(std::to_string(job_id));

    djs::GetJobStatusReply reply;
    grpc::ClientContext context;
    add_auth(context);

    grpc::Status status = stub->GetJobStatus(&context, request, &reply);

    if (!status.ok()) {
        std::cout << "RPC failed: " << status.error_message() << "\n";
        return;
    }

    db.cache_job_result(job_id,
                        reply.status(),
                        reply.has_result(),
                        reply.success(),
                        reply.result_message(),
                        reply.artifact_url(),
                        reply.completed_at());

    auto r = db.get_cached_job_result(job_id);
    print_job_result(r, false);
}

void SchedulerClient::RegisterClient(const std::string& hostname, const std::string& token) {

    token_ = token;

    djs::RegisterClientRequest request;
    request.set_hostname(hostname);

    djs::RegisterClientResponse reply;
    grpc::ClientContext context;
    add_auth(context);

    grpc::Status status = stub->RegisterClient(&context, request, &reply);

    if (status.ok()) {
        if (reply.ok()) {
            db.insert_client(hostname, reply.client_id());
            db.save_token(token_);
            std::cout << "message: " << reply.message() << "\n";
            std::cout << "client_id: " << reply.client_id() << "\n";
        } else {
            std::cout << "registration failed: " << reply.message() << "\n";
        }
    } else {
        std::cout << "RPC failed: " << status.error_message() << "\n";
    }
}
