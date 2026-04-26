#pragma once

#include "scheduler.grpc.pb.h"
#include <grpcpp/grpcpp.h>

#include <memory>
#include <string>

class SchedulerClient {
  private:
    std::unique_ptr<djs::SchedulerService::Stub> stub;

  public:
    explicit SchedulerClient(std::shared_ptr<grpc::Channel> channel);

    void SubmitJob(const std::string& job_id, const std::string& payload);
};
