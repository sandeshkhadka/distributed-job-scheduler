#include "mixed_executor.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <thread>
#include <vector>

JobResult MixedLoadExecutor::execute(const std::map<std::string, std::string>& params) {
    int cpu_cores = 1;
    int mem_mb = 64;
    int io_mb = 32;
    int duration_ms = 10000;

    auto it = params.find("cpu_cores");
    if (it != params.end())
        cpu_cores = std::stoi(it->second);
    it = params.find("mem_mb");
    if (it != params.end())
        mem_mb = std::stoi(it->second);
    it = params.find("io_mb");
    if (it != params.end())
        io_mb = std::stoi(it->second);
    it = params.find("duration_ms");
    if (it != params.end())
        duration_ms = std::stoi(it->second);

    std::atomic<bool> stop{false};
    std::vector<std::thread> threads;

    // CPU burn threads
    for (int i = 0; i < cpu_cores; ++i) {
        threads.emplace_back([&stop, duration_ms] {
            auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);
            while (std::chrono::steady_clock::now() < end && !stop.load()) {
                volatile double x = std::sqrt(3.14159) * std::sin(1.0) + std::cos(1.0);
                (void)x;
            }
        });
    }

    // Memory load
    threads.emplace_back([&stop, mem_mb, duration_ms] {
        const int chunk = 1024 * 1024;
        std::vector<char*> blocks;
        for (int i = 0; i < mem_mb; ++i) {
            if (stop.load())
                break;
            try {
                char* p = new char[chunk];
                std::memset(p, 0xFF, chunk);
                blocks.push_back(p);
            } catch (...) {
                break;
            }
        }
        auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);
        while (std::chrono::steady_clock::now() < end && !stop.load()) {
            for (auto p : blocks)
                std::memset(p, 0xFF, chunk);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        for (auto p : blocks)
            delete[] p;
    });

    // I/O load
    threads.emplace_back([&stop, io_mb, duration_ms] {
        std::string path = "/tmp/mixed_io_" + std::to_string(time(nullptr)) + ".tmp";
        long bytes = static_cast<long>(io_mb) * 1024 * 1024;
        auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);
        std::vector<char> buf(4096, 'Z');
        while (std::chrono::steady_clock::now() < end && !stop.load()) {
            {
                std::ofstream out(path, std::ios::binary);
                long written = 0;
                while (written < bytes) {
                    out.write(buf.data(), buf.size());
                    written += buf.size();
                }
            }
            {
                std::ifstream in(path, std::ios::binary);
                std::vector<char> rb(4096);
                while (in.read(rb.data(), rb.size())) {
                }
            }
        }
        std::remove(path.c_str());
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    stop.store(true);

    for (auto& t : threads) {
        if (t.joinable())
            t.join();
    }

    return {true,
            "completed: mixed_load cpu=" + std::to_string(cpu_cores) +
                " mem=" + std::to_string(mem_mb) + "MB io=" + std::to_string(io_mb) + "MB",
            ""};
}
