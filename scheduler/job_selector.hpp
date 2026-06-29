#pragma once
#include "database.h"
#include "idb.hpp"
#include "types.hpp"
#include <string>
#include <vector>

class JobSelector {
  public:
    virtual ~JobSelector() = default;
    virtual std::string name() const = 0;
    virtual Job
    select_job(const Worker& worker, const std::vector<Job>& jobs, ISchedulerDatabase& db) = 0;
};
