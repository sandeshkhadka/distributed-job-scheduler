#pragma once
#include "job_selector.hpp"
#include "scheduler_db.h"

class SJFSelector : public JobSelector {
  public:
    std::string name() const override;
    Job
    select_job(const Worker& worker, const std::vector<Job>& jobs, SchedulerDatabase& db) override;
};
