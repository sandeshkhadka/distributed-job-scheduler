#include "CliParser.hpp"
#include <argparse.hpp>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

CliParser::CliParser(int argc, char* argv[]) : _dist_cli("dist_cli") {
    this->_actions.push_back("submit");
    argparse::ArgumentParser submit("submit");
    submit.add_description("Submit a job to the scheduler");
    submit.add_argument("job");
    // presence of this flag indicates that we're being given a job file
    submit.add_argument("--type, -t").required().default_value("build_docker_image").help("");

    this->_actions.push_back("query");
    argparse::ArgumentParser query("query");
    query.add_description("Query the status of a job with given JOB_ID");
    query.add_argument("job_id").help("Job id of the job to get the status of").scan<'i', int>();

    this->_actions.push_back("cancel");
    argparse::ArgumentParser cancel("cancel");
    cancel.add_description("cancel the status of a job with given JOB_ID");
    cancel.add_argument("job_id").help("Job id of the job to cancel").scan<'i', int>();

    this->_actions.push_back("list");
    argparse::ArgumentParser list("list");
    list.add_description("List the jobs");

    this->_dist_cli.add_subparser(submit);
    this->_dist_cli.add_subparser(query);
    this->_dist_cli.add_subparser(cancel);
    this->_dist_cli.add_subparser(list);

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

void CliParser::print_help() { std::cout << _dist_cli; }
