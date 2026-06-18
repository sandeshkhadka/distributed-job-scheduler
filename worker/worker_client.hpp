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
    std::string token_;

    void add_auth(grpc::ClientContext& context);
    int do_register();

  public:
    explicit WorkerClient(std::shared_ptr<grpc::Channel> channel);

    int Register();
    int Register(const std::string& token);

    void GetJob();
};
