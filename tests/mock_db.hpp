#pragma once
#include "idb.hpp"
#include "types.hpp"
#include <map>
#include <optional>
#include <string>

class MockDatabase : public ISchedulerDatabase {
  public:
    bool valid_token = true;
    std::string expected_token_type = "worker";
    std::optional<WorkerMetricsBrief> metrics;
    std::map<std::string, double> durations;
    std::map<std::string, JobTypeSpikes> spikes;

    bool is_token_valid(const std::string& token, const std::string& token_type) override {
        return valid_token && token_type == expected_token_type;
    }

    std::optional<WorkerMetricsBrief> get_worker_latest_metrics(int worker_id) override {
        return metrics;
    }

    std::map<std::string, double> get_avg_durations() override { return durations; }

    std::map<std::string, JobTypeSpikes> get_avg_spikes() override { return spikes; }
};
