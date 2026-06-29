#pragma once
#include "scheduler.pb.h"
#include <mutex>
#include <unordered_map>
#include <vector>

class EbpfMetricStore {
  public:
    void add_snapshot(int job_id, const djs::JobEbpfMetrics& snap);
    std::vector<djs::JobEbpfMetrics> drain(int job_id);
    std::vector<int> active_job_ids();

  private:
    std::mutex mutex_;
    std::unordered_map<int, std::vector<djs::JobEbpfMetrics>> metrics_;
};
