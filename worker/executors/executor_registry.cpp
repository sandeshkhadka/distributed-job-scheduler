#include "executors/executor_registry.hpp"
#include "executors/mixed_executor.hpp"
#include "executors/stress_cpu_executor.hpp"
#include "executors/stress_io_executor.hpp"
#include "executors/stress_mem_executor.hpp"
#include <functional>
#include <unordered_map>

using Factory = std::function<std::unique_ptr<JobExecutor>()>;

static std::unordered_map<std::string, Factory>& factories() {
    static std::unordered_map<std::string, Factory> map;
    return map;
}

void ExecutorRegistry::init() {
    factories()["stress_cpu"] = [] { return std::make_unique<StressCpuExecutor>(); };
    factories()["stress_mem"] = [] { return std::make_unique<StressMemExecutor>(); };
    factories()["stress_io"] = [] { return std::make_unique<StressIOExecutor>(); };
    factories()["mixed_load"] = [] { return std::make_unique<MixedLoadExecutor>(); };
}

std::unique_ptr<JobExecutor> ExecutorRegistry::create(const std::string& type) {
    auto it = factories().find(type);
    if (it != factories().end()) {
        return it->second();
    }
    return nullptr;
}
