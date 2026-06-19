#pragma once
#include "executors/job_executor.hpp"

class StressCpuExecutor : public JobExecutor {
  public:
    std::string type() const override { return "stress_cpu"; }
    JobResult execute(const std::map<std::string, std::string>& params) override;
};
