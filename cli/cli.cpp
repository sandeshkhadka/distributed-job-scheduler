#include "./scheduler_client.h"
#include "logger.h"
#include "utils/CliParser.hpp"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <map>

using Logger = DJS::Logger;

int main(int argc, char* argv[]) {
    CliParser cli_parser(argc, argv);
    auto action = cli_parser.get_action();
    auto possible_actions = cli_parser.get_possible_actions();

    if (action == "") {
        std::cout << "No supported action has been provided!\n";
        cli_parser.print_help();
    }

    std::string scheduler_addr = cli_parser.get_scheduler_address();
    std::shared_ptr<grpc::Channel> channel =
        grpc::CreateChannel(scheduler_addr, grpc::InsecureChannelCredentials());

    std::string hostname = scheduler_addr.substr(0, scheduler_addr.find(':'));

    if (action == "submit") {
        SchedulerClient client(channel);

        std::string job_type = cli_parser.get_value("--type");
        auto param_list = cli_parser.get_list("--param");

        std::map<std::string, std::string> params;
        for (const auto& p : param_list) {
            auto eq = p.find('=');
            if (eq != std::string::npos) {
                params[p.substr(0, eq)] = p.substr(eq + 1);
            } else {
                Logger::Error("Invalid param format (expected key=value): " + p);
            }
        }

        client.SubmitJob(job_type, params);
    }

    if (action == "register") {
        std::string token = cli_parser.get_value("--token");
        std::cout << "Registering...\n";
        SchedulerClient client(channel);
        client.RegisterClient(hostname, token);
    }

    if (action == "jobs") {
        bool refresh = cli_parser.has_flag("--refresh");
        SchedulerClient client(channel);
        client.ListMyJobs(refresh);
    }

    if (action == "result") {
        std::string job_id_str = cli_parser.get_value("--id");
        if (job_id_str.empty()) {
            std::cout << "Error: --id <job_id> is required for 'result' action\n";
            return 1;
        }
        bool refresh = cli_parser.has_flag("--refresh");
        int job_id = std::stoi(job_id_str);
        SchedulerClient client(channel);
        client.GetJobResult(job_id, refresh);
    }
}
