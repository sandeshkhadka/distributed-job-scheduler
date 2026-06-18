#include "stress_cpu_executor.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>
#include <vector>

JobResult StressCpuExecutor::execute(const std::map<std::string, std::string>& params) {
    int cores = 1;
    int duration_ms = 5000;

    auto it = params.find("cores");
    if (it != params.end())
        cores = std::stoi(it->second);
    it = params.find("duration_ms");
    if (it != params.end())
        duration_ms = std::stoi(it->second);

    std::atomic<bool> stop{false};
    std::vector<std::thread> workers;

    for (int i = 0; i < cores; ++i) {
        workers.emplace_back([&stop, duration_ms] {
            auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);
            while (std::chrono::steady_clock::now() < end && !stop.load()) {
                volatile double x = 3.14159;
                for (int j = 0; j < 10000; ++j) {
                    x = std::sqrt(std::sin(x) * std::cos(x)) + std::tan(x);
                }
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    stop.store(true);

    for (auto& t : workers) {
        if (t.joinable())
            t.join();
    }

    return {true,
            "completed: stress_cpu " + std::to_string(cores) + " cores for " +
                std::to_string(duration_ms) + "ms",
            ""};
}
