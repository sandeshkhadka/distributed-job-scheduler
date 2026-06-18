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

  public:
    explicit SchedulerClient(std::shared_ptr<grpc::Channel> channel);
    void RegisterClient(const std::string& hostname);

    void SubmitJob(const std::string& job_type, const std::map<std::string, std::string>& params);
};
