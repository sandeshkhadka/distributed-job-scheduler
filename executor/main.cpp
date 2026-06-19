#include "executors/executor_registry.hpp"
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    std::string type;
    std::map<std::string, std::string> params;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--type" && i + 1 < argc) {
            type = argv[++i];
        } else if (arg == "--param" && i + 1 < argc) {
            std::string kv = argv[++i];
            auto eq = kv.find('=');
            if (eq != std::string::npos) {
                params[kv.substr(0, eq)] = kv.substr(eq + 1);
            }
        }
    }

    if (type.empty()) {
        std::cerr << "{\"success\":false,\"message\":\"missing --type\"}" << std::endl;
        return 1;
    }

    ExecutorRegistry::init();
    auto executor = ExecutorRegistry::create(type);
    if (!executor) {
        std::cerr << "{\"success\":false,\"message\":\"unknown type: " << type << "\"}"
                  << std::endl;
        return 1;
    }

    auto result = executor->execute(params);

    std::cout << "{\"success\":" << (result.success ? "true" : "false") << ",\"message\":\""
              << result.message << "\",\"artifact_url\":\"" << result.artifact_url << "\"}"
              << std::endl;

    return result.success ? 0 : 1;
}
