#pragma once
#include "database.h"

struct Worker {
    int server_id;
    std::string name;
};

int get_registered_worker_id_cb(void* data, int argc, char** argv, char** col_name);

class WorkerDatabase : public Database {
  public:
    WorkerDatabase();

    int get_registered_worker_id(); // returns -1 if no worker is registered
    int insert_worker(const Worker& worker);
};
