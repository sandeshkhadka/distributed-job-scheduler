#pragma once
#include "database.h"
#include <string>

struct Worker {
    int server_id;
    std::string name;
};

int get_registered_worker_id_cb(void* data, int argc, char** argv, char** col_name);
int get_token_cb(void* data, int argc, char** argv, char** col_name);

class WorkerDatabase : public Database {
  public:
    WorkerDatabase();

    int get_registered_worker_id();
    int insert_worker(const Worker& worker);

    void save_token(const std::string& token);
    std::string get_token();
};
