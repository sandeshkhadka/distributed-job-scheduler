#pragma once
#include "database.h"
#include "scheduler_db.h"
#include <string>
#include <vector>

class JobSelector {
  public:
    virtual ~JobSelector() = default;
    virtual std::string name() const = 0;
    virtual Job
    select_job(const Worker& worker, const std::vector<Job>& jobs, SchedulerDatabase& db) = 0;
};
