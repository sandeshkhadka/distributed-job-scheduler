#pragma once

#include "logger.h"
#include "scheduler.grpc.pb.h"
#include "worker_db.hpp"
#include <grpcpp/grpcpp.h>

using Logger = DJS::Logger;

class WorkerClient {
  private:
    std::unique_ptr<djs::SchedulerService::Stub> stub_;
    int worker_id{-1};
    WorkerDatabase db;

  public:
    explicit WorkerClient(std::shared_ptr<grpc::Channel> channel);

    int Register();
    void GetJob();
    int execute_job(); // returns the status of executed job
                       // TODO: return the actual result
};
