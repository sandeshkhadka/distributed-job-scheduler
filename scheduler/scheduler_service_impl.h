#pragma once

#include "scheduler.grpc.pb.h"
#include "scheduler.pb.h"
#include "scheduler_db.h"
#include <grpcpp/support/status.h>
#include <string>
#include <unordered_map>

class SchedulerServiceImpl final : public djs::SchedulerService::Service {
  private:
    std::unordered_map<std::string, std::string> jobs;
    std::unordered_map<std::string, std::string> active_workers;
    SchedulerDatabase db;

  public:
    grpc::Status SubmitJob(grpc::ServerContext* context,
                           const djs::SubmitJobRequest* request,
                           djs::SubmitJobReply* reply) override;

    // Add the new RPC method override
    grpc::Status RegisterWorker(grpc::ServerContext* context,
                                const djs::RegisterWorkerRequest* request,
                                djs::RegisterWorkerReply* reply) override;

    grpc::Status GetJob(grpc::ServerContext* context,
                        const djs::GetJobRequest* request,
                        djs::GetJobResponse* reply) override;

    grpc::Status RegisterClient(grpc::ServerContext* context,
                                const djs::RegisterClientRequest* request,
                                djs::RegisterClientResponse* response) override;

  private:
    // The main function to select a job for a worker
    Job select_job(const Worker& worker, const std::vector<Job>& jobs);
};
