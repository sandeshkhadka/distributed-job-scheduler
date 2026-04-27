#pragma once
#include "database.h"

struct Client {
    int client_id;
    std::string name;
};

class CliDatabase : public Database {
  public:
    CliDatabase();

    int insert_client(const Client& client);
    int get_active_client();
};
