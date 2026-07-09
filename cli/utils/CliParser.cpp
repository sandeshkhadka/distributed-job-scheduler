#include "CliParser.hpp"
#include <argparse.hpp>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

CliParser::CliParser(int argc, char* argv[]) : _dist_cli("dist_cli") {
    _logger = DJS::Logger();
    this->_actions.push_back("submit");
    _submit_cmd.add_description("Submit a job to the scheduler");
    _submit_cmd.add_argument("--type", "-t")
        .required()
        .help("Job type (e.g. stress_cpu, stress_mem, mixed_load)");
    _submit_cmd.add_argument("--param", "-P")
        .append()
        .help("key=value parameter for the job (repeatable)");

    this->_actions.push_back("register");
    _register_cmd.add_description("Register the client to the scheduler");
    _register_cmd.add_argument("--token", "-k").required().help("Auth token for the client");

    this->_actions.push_back("query");
    _query_cmd.add_description("Query the status of a job with given JOB_ID");
    _query_cmd.add_argument("job_id")
        .help("Job id of the job to get the status of")
        .scan<'i', int>();

    this->_actions.push_back("cancel");
    _cancel_cmd.add_description("cancel the status of a job with given JOB_ID");
    _cancel_cmd.add_argument("job_id").help("Job id of the job to cancel").scan<'i', int>();

    this->_actions.push_back("list");
    _list_cmd.add_description("List the jobs");

    this->_actions.push_back("jobs");
    _jobs_cmd.add_description("List your submitted jobs");
    _jobs_cmd.add_argument("--refresh").flag().help("Force refresh from scheduler");

    this->_actions.push_back("result");
    _result_cmd.add_description("Get the result of a job by job ID");
    _result_cmd.add_argument("--id").required().help("Job ID to query");
    _result_cmd.add_argument("--refresh").flag().help("Force refresh from scheduler");

    this->_dist_cli.add_argument("--scheduler", "-s")
        .default_value(std::string("localhost:50051"))
        .help("Scheduler address (default: localhost:50051)");

    this->_dist_cli.add_subparser(_submit_cmd);
    this->_dist_cli.add_subparser(_register_cmd);
    this->_dist_cli.add_subparser(_query_cmd);
    this->_dist_cli.add_subparser(_cancel_cmd);
    this->_dist_cli.add_subparser(_list_cmd);
    this->_dist_cli.add_subparser(_jobs_cmd);
    this->_dist_cli.add_subparser(_result_cmd);

    try {
        this->_dist_cli.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cout << "hola";
        std::cerr << err.what() << std::endl << std::endl;
        std::cerr << this->_dist_cli;
        std::exit(1);
    }
}

std::string CliParser::get_action() {
    for (const auto& action : _actions) {
        if (_dist_cli.is_subcommand_used(action)) {
            return action;
        }
    }

    return "";
}

std::vector<std::string> CliParser::get_possible_actions() { return _actions; }

std::string CliParser::get_value(const std::string& flag) {
    std::string value;
    try {
        if (_dist_cli.is_subcommand_used(_submit_cmd)) {
            return _submit_cmd.get(flag);
        }
        if (_dist_cli.is_subcommand_used(_register_cmd)) {
            return _register_cmd.get(flag);
        }
        if (_dist_cli.is_subcommand_used(_result_cmd)) {
            return _result_cmd.get(flag);
        }
    } catch (std::exception& e) {
        _logger.Error("Invalid value for flag: " + flag + "Error: " + e.what());
        value = "";
    }
    return value;
}

std::vector<std::string> CliParser::get_list(const std::string& flag) {
    if (_dist_cli.is_subcommand_used(_submit_cmd)) {
        try {
            return _submit_cmd.get<std::vector<std::string>>(flag);
        } catch (const std::logic_error&) {
            return {};
        }
    }
    return {};
}

bool CliParser::has_flag(const std::string& flag) {
    try {
        if (_dist_cli.is_subcommand_used(_jobs_cmd)) {
            return _jobs_cmd.get<bool>(flag);
        }
        if (_dist_cli.is_subcommand_used(_result_cmd)) {
            return _result_cmd.get<bool>(flag);
        }
    } catch (...) {
        return false;
    }
    return false;
}

std::string CliParser::get_scheduler_address() const {
    return _dist_cli.get<std::string>("--scheduler");
}

void CliParser::print_help() { std::cout << _dist_cli; }
