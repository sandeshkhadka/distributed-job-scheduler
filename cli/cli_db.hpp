#pragma once
#include "database.h"

class CliDatabase : public Database {
  public:
    CliDatabase();
    int insert_client(const Client& client) override;
};
