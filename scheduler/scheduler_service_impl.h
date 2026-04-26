#pragma once

#include "scheduler.grpc.pb.h"
#include <string>
#include <unordered_map>

class SchedulerServiceImpl final : public djs::SchedulerService::Service {
  private:
    std::unordered_map<std::string, std::string> jobs;
    std::unordered_map<std::string, std::string> active_workers;

  public:
    grpc::Status SubmitJob(grpc::ServerContext* context,
                           const djs::SubmitJobRequest* request,
                           djs::SubmitJobReply* reply) override;

    // Add the new RPC method override
    grpc::Status RegisterWorker(grpc::ServerContext* context,
                                const djs::RegisterWorkerRequest* request,
                                djs::RegisterWorkerReply* reply) override;
};
