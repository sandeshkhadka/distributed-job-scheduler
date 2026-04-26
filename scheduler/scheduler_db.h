#pragma once
#include "database.h"

struct Worker {
    int id;
    int cpu_cores;
    float mem_size;
    float disk_size;
    float cpu_freq;

    std::string os;
    std::string name;
    std::string status;
};

struct WorkerJob {
    int worker_id;
    int job_id;
};

int insert_worker_cb(void* data, int argc, char** argv, char** col_name);
int get_worker_cb(void* data, int argc, char** argv, char** col_name);

class SchedulerDatabase : public Database {
  public:
    SchedulerDatabase();

    Worker get_worker_by_id(int worker_id);
    std::vector<Worker> get_worker_by_status(const std::string& status);
    std::vector<Worker> get_all_workers();

    int insert_worker(const Worker& worker);

    void update_worker_status(int worker_id, const std::string& status);
    void delete_worker(int worker_id);

    void insert_worker_job(const WorkerJob& worker_job);
    std::vector<Job> get_worker_jobs(int worker_id);
    Worker get_job_worker(int job_id);
};
