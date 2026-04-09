#ifndef CLIPARSER_H
#define CLIPARSER_H

#include "argparse.hpp"
#include <string>
#include <vector>
class CliParser {
  private:
    argparse::ArgumentParser _dist_cli;
    std::vector<std::string> _actions;

  public:
    CliParser(int argc, char* argv[]); // Constructor declaration
    std::string get_action();          // Member function prototype
    std::vector<std::string> get_possible_actions();
    void print_help();
};

#endif
