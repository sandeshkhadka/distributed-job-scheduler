#ifndef CLIPARSER_H
#define CLIPARSER_H

#include "argparse.hpp"
#include "logger.h"
#include <string>
#include <vector>
using ArgumentParser = argparse::ArgumentParser;
class CliParser {
  private:
    ArgumentParser _dist_cli;
    ArgumentParser _submit_cmd{"submit"};
    ArgumentParser _register_cmd{"register"};
    ArgumentParser _query_cmd{"query"};
    ArgumentParser _cancel_cmd{"cancel"};
    ArgumentParser _list_cmd{"list"};
    ArgumentParser _jobs_cmd{"jobs"};
    ArgumentParser _result_cmd{"result"};
    std::vector<std::string> _actions;
    DJS::Logger _logger;

  public:
    CliParser(int argc, char* argv[]); // Constructor declaration
    std::string get_action();          // Member function prototype
    std::vector<std::string> get_possible_actions();
    void print_help();
    std::string get_value(const std::string&);
    std::vector<std::string> get_list(const std::string&);
    bool has_flag(const std::string& flag);
};

#endif
