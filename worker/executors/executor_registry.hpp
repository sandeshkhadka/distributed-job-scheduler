#pragma once
#include "job_executor.hpp"
#include <memory>
#include <string>

class ExecutorRegistry {
  public:
    static void init();
    static std::unique_ptr<JobExecutor> create(const std::string& type);
};
