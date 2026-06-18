#pragma once
#include "executors/job_executor.hpp"

class StressIOExecutor : public JobExecutor {
  public:
    std::string type() const override { return "stress_io"; }
    JobResult execute(const std::map<std::string, std::string>& params) override;
};
