#pragma once
#include "executors/job_executor.hpp"

class MixedLoadExecutor : public JobExecutor {
  public:
    std::string type() const override { return "mixed_load"; }
    JobResult execute(const std::map<std::string, std::string>& params) override;
};
