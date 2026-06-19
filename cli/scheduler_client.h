#pragma once

#include "cli_db.hpp"
#include "scheduler.grpc.pb.h"
#include <grpcpp/grpcpp.h>

#include <memory>
#include <string>

class SchedulerClient {
  private:
    std::unique_ptr<djs::SchedulerService::Stub> stub;
    CliDatabase db;
    std::string token_;

    void add_auth(grpc::ClientContext& context);

  public:
    explicit SchedulerClient(std::shared_ptr<grpc::Channel> channel);

    void RegisterClient(const std::string& hostname, const std::string& token);
    void SubmitJob(const std::string& job_type, const std::map<std::string, std::string>& params);
};
