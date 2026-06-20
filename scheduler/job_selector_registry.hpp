#pragma once
#include "job_selector.hpp"
#include <map>
#include <memory>
#include <string>

class JobSelectorRegistry {
  public:
    void init_all();
    void register_selector(const std::string& name, std::unique_ptr<JobSelector> (*creator)());
    std::unique_ptr<JobSelector> create(const std::string& name) const;

  private:
    std::map<std::string, std::unique_ptr<JobSelector> (*)()> creators_;
};

template <typename T> std::unique_ptr<JobSelector> make_selector() { return std::make_unique<T>(); }
