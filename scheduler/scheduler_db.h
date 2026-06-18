#pragma once
#include "database.h"
#include <string>

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

struct Client {
    // int id;
    std::string name; // hostname
    std::string status;
};

struct AuthToken {
    int id;
    std::string token;
    std::string description;
    std::string token_type;
    bool active;
    std::string created_at;
};

int insert_worker_cb(void* data, int argc, char** argv, char** col_name);
int get_worker_cb(void* data, int argc, char** argv, char** col_name);

int token_exists_cb(void* data, int argc, char** argv, char** col_name);
int token_id_cb(void* data, int argc, char** argv, char** col_name);

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

    int insert_client(const Client& client);

    bool is_token_valid(const std::string& token, const std::string& token_type);
    int get_token_id(const std::string& token);
    void record_client_token_usage(int client_id, int token_id);
};
