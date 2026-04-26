#include "./scheduler_client.h"
#include "logger.h"
#include "utils/CliParser.hpp"
#include <grpcpp/grpcpp.h>
#include <iostream>

using Logger = DJS::Logger;

int main(int argc, char* argv[]) {
    CliParser cli_parser(argc, argv);
    auto action = cli_parser.get_action();
    auto possible_actions = cli_parser.get_possible_actions();

    if (action == "") {
        std::cout << "No supported action has been provided!\n";
        cli_parser.print_help();
    }

    if (action == "submit") {
        SchedulerClient client(
            grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

        std::string payload = cli_parser.get_value("--payload");
        client.SubmitJob(payload);
    }
}
