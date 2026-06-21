#pragma once

#include "executors/job_executor.hpp"
#include "logger.h"
#include "metrics_collector.hpp"
#include "scheduler.grpc.pb.h"
#include "worker_db.hpp"
#include <grpcpp/grpcpp.h>
#include <optional>

using Logger = DJS::Logger;

struct ReceivedJob {
    int job_id;
    std::string job_type;
    std::map<std::string, std::string> params;
};

class WorkerClient {
  private:
    std::unique_ptr<djs::SchedulerService::Stub> stub_;
    int worker_id{-1};
    std::string token_;

    void add_auth(grpc::ClientContext& context);
    int do_register();

  public:
    WorkerDatabase db;

    explicit WorkerClient(std::shared_ptr<grpc::Channel> channel);

    int Register();
    int Register(const std::string& token);

    std::optional<ReceivedJob> GetJob();
    bool confirm_job_received(int job_id);
    bool report_job_started(int job_id);
    void store_job_result(int job_id, const JobResult& result);
    void report_pending_results();
    void report_worker_metrics(const WorkerMetricsData& metrics);
};
