#pragma once

#include "scheduler.grpc.pb.h"
#include <string>
#include <unordered_map>

class SchedulerServiceImpl final : public djs::SchedulerService::Service {
  private:
    std::unordered_map<std::string, std::string> jobs;

  public:
    grpc::Status SubmitJob(grpc::ServerContext* context,
                           const djs::SubmitJobRequest* request,
                           djs::SubmitJobReply* reply) override;
};
