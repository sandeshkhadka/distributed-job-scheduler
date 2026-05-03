#pragma once
#include "database.h"

class WorkerDatabase : public Database {
  public:
    WorkerDatabase();

    int insert_worker(const Worker& worker) override;
};
