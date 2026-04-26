#pragma once

#include "logger.h"
#include "scheduler.grpc.pb.h"
#include <grpcpp/grpcpp.h>

using Logger = DJS::Logger;

class WorkerClient {
  private:
    std::unique_ptr<djs::SchedulerService::Stub> stub_;

  public:
    explicit WorkerClient(std::shared_ptr<grpc::Channel> channel);

    void Register(const std::string& worker_id, const std::string& host, int port);
};
