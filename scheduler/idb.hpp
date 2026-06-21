#pragma once
#include "types.hpp"
#include <map>
#include <optional>
#include <string>

class ISchedulerDatabase {
  public:
    virtual ~ISchedulerDatabase() = default;
    virtual bool is_token_valid(const std::string& token, const std::string& token_type) = 0;
    virtual std::optional<WorkerMetricsBrief> get_worker_latest_metrics(int worker_id) = 0;
    virtual std::map<std::string, double> get_avg_durations() = 0;
    virtual std::map<std::string, JobTypeSpikes> get_avg_spikes() = 0;
};
