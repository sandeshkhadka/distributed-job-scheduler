#pragma once
#include <map>
#include <string>

struct JobResult {
    bool success;
    std::string message;
    std::string artifact_url;
};

class JobExecutor {
  public:
    virtual ~JobExecutor() = default;
    virtual std::string type() const = 0;
    virtual JobResult execute(const std::map<std::string, std::string>& params) = 0;
};
