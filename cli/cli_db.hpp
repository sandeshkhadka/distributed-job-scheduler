#pragma once
#include "database.h"

struct Client {
    int client_id;
    std::string name;
};

int get_active_client_callback(void* data, int, char** values, char**);

class CliDatabase : public Database {
  public:
    CliDatabase();

    int insert_client(const std::string& name, int client_id);
    int get_active_client();
};
