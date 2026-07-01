#pragma once

#include "job_selector.hpp"
#include "job_selector_registry.hpp"
#include "scheduler.grpc.pb.h"
#include "scheduler.pb.h"
#include <grpcpp/support/status.h>
#include <memory>
#include <string>

#if defined(USE_PG) && USE_PG
#include "pg_scheduler_db.hpp"
using SchedulerDb = PgSchedulerDatabase;
#else
#include "scheduler_db.h"
using SchedulerDb = SchedulerDatabase;
#endif

class SchedulerServiceImpl final : public djs::SchedulerService::Service {
  private:
    SchedulerDb db;

  public:
    std::unique_ptr<JobSelector> job_selector_;

    grpc::Status SubmitJob(grpc::ServerContext* context,
                           const djs::SubmitJobRequest* request,
                           djs::SubmitJobReply* reply) override;

    grpc::Status RegisterWorker(grpc::ServerContext* context,
                                const djs::RegisterWorkerRequest* request,
                                djs::RegisterWorkerReply* reply) override;

    grpc::Status GetJob(grpc::ServerContext* context,
                        const djs::GetJobRequest* request,
                        djs::GetJobResponse* reply) override;

    grpc::Status GetJobStatus(grpc::ServerContext* context,
                              const djs::GetJobStatusRequest* request,
                              djs::GetJobStatusReply* reply) override;

    grpc::Status RegisterClient(grpc::ServerContext* context,
                                const djs::RegisterClientRequest* request,
                                djs::RegisterClientResponse* response) override;

    grpc::Status ReportJobResult(grpc::ServerContext* context,
                                 const djs::ReportJobResultRequest* request,
                                 djs::ReportJobResultReply* reply) override;

    grpc::Status ConfirmJobReceived(grpc::ServerContext* context,
                                    const djs::ConfirmJobReceivedRequest* request,
                                    djs::ConfirmJobReceivedResponse* reply) override;

    grpc::Status ReportJobStarted(grpc::ServerContext* context,
                                  const djs::ReportJobStartedRequest* request,
                                  djs::ReportJobStartedResponse* reply) override;

    grpc::Status ReportWorkerMetrics(grpc::ServerContext* context,
                                     const djs::WorkerMetrics* request,
                                     djs::ReportWorkerMetricsResponse* reply) override;

    grpc::Status ReportJobEbpfMetrics(grpc::ServerContext* context,
                                      const djs::ReportJobEbpfMetricsRequest* request,
                                      djs::ReportJobEbpfMetricsResponse* reply) override;

    grpc::Status GetJobTimeseries(grpc::ServerContext* context,
                                  const djs::GetJobTimeseriesRequest* request,
                                  djs::GetJobTimeseriesResponse* reply) override;
    grpc::Status GetClientJobs(grpc::ServerContext* context,
                               const djs::GetClientJobsRequest* request,
                               djs::GetClientJobsResponse* reply) override;
};
