#pragma once
#include <string>
#include <vector>

struct Job {
    int id;
    int client_id;
    std::string payload;
    std::string status;
    std::string created_at;
    std::string updated_at;
};

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
    int id;
    std::string name; // hostname
    std::string status;
};

// helper callbacks
int get_job_cb(void* container, int argc, char** argv, char** col_name);
int create_job_cb(void* container, int argc, char** argv, char** col_name);
int get_worker_cb(void* data, int argc, char** argv, char** col_name);
int get_registered_worker_id_cb(void* data, int argc, char** argv, char** col_name);
int get_active_client_callback(void* data, int, char** values, char**);

class Database {
  public:
    Database(const std::string& db_name);

    // job
    int insert_job(const std::string& payload, int client_id);
    Job get_job_by_id(int job_id);
    std::vector<Job> get_job_by_client_id(int client_id);
    std::vector<Job> get_all_jobs();
    std::vector<Job> get_jobs_by_status(const std::string& status);

    // worker
    Worker get_worker_by_id(int worker_id);
    std::vector<Worker> get_worker_by_status(const std::string& status);
    std::vector<Worker> get_all_workers();

    virtual int insert_worker(const Worker& worker);

    void update_worker_status(int worker_id, const std::string& status);
    void delete_worker(int worker_id);

    void insert_worker_job(const WorkerJob& worker_job);
    std::vector<Job> get_worker_jobs(int worker_id);
    Worker get_job_worker(int job_id);

    virtual int insert_client(const Client& client);

    // for worker clients
    int get_registered_worker_id(); // returns -1 if no worker is registered

    // for clients
    int get_active_client();
};
