#include "stress_mem_executor.hpp"
#include <chrono>
#include <cstring>
#include <thread>
#include <vector>

JobResult StressMemExecutor::execute(const std::map<std::string, std::string>& params) {
    int mb = 128;
    int duration_ms = 5000;

    auto it = params.find("mb");
    if (it != params.end())
        mb = std::stoi(it->second);
    it = params.find("duration_ms");
    if (it != params.end())
        duration_ms = std::stoi(it->second);

    const int chunk = 1024 * 1024;
    std::vector<char*> blocks;

    try {
        for (int i = 0; i < mb; ++i) {
            char* p = new char[chunk];
            std::memset(p, 0xFF, chunk);
            blocks.push_back(p);
        }
    } catch (const std::bad_alloc&) {
        for (auto p : blocks)
            delete[] p;
        return {false, "failed: could not allocate " + std::to_string(mb) + "MB", ""};
    }

    for (int i = 0; i < mb; ++i) {
        std::memset(blocks[i], 0xFF, chunk);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));

    for (auto p : blocks)
        delete[] p;

    return {true,
            "completed: stress_mem " + std::to_string(mb) + "MB for " +
                std::to_string(duration_ms) + "ms",
            ""};
}
