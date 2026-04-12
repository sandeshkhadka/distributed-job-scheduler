#include "logger.h"
#include "utils/CliParser.hpp"
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

    for (auto possible_action : possible_actions) {
        if (action == possible_action) {
            std::cout << "The user has asked for the action: " << action;
        }
    }
}
