#pragma once
#include "job_selector.hpp"

class FCFSSelector : public JobSelector {
  public:
    std::string name() const override;
    Job select_job(const Worker& worker, const std::vector<Job>& jobs) override;
};
