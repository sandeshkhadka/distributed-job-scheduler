#pragma once
#include "executors/job_executor.hpp"

class StressMemExecutor : public JobExecutor {
  public:
    std::string type() const override { return "stress_mem"; }
    JobResult execute(const std::map<std::string, std::string>& params) override;
};
