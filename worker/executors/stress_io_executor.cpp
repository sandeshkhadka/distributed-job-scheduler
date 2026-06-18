#include "stress_io_executor.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <thread>
#include <vector>

JobResult StressIOExecutor::execute(const std::map<std::string, std::string>& params) {
    int mb = 64;
    int block_size = 4096;
    int duration_ms = 5000;
    std::string mode = "rw";

    auto it = params.find("mb");
    if (it != params.end())
        mb = std::stoi(it->second);
    it = params.find("block_size");
    if (it != params.end())
        block_size = std::stoi(it->second);
    it = params.find("duration_ms");
    if (it != params.end())
        duration_ms = std::stoi(it->second);
    it = params.find("mode");
    if (it != params.end())
        mode = it->second;

    std::string path = "/tmp/stress_io_" + std::to_string(time(nullptr)) + ".tmp";
    long total_bytes = static_cast<long>(mb) * 1024 * 1024;
    int blocks = total_bytes / block_size;
    std::vector<char> buf(block_size, 'A');

    if (mode == "write" || mode == "rw") {
        std::ofstream out(path, std::ios::binary);
        if (!out)
            return {false, "failed: cannot open " + path + " for writing", ""};
        for (int i = 0; i < blocks; ++i) {
            out.write(buf.data(), block_size);
        }
        out.close();
    }

    if (mode == "read" || mode == "rw") {
        std::ifstream in(path, std::ios::binary);
        if (!in)
            return {false, "failed: cannot open " + path + " for reading", ""};
        std::vector<char> read_buf(block_size);
        for (int i = 0; i < blocks; ++i) {
            in.read(read_buf.data(), block_size);
        }
        in.close();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    std::remove(path.c_str());

    return {true, "completed: stress_io " + std::to_string(mb) + "MB mode=" + mode, ""};
}
