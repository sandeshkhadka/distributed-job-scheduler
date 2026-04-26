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
int get_job_cb(void* container, int argc, char** argv, char** col_name);
int create_job_cb(void* container, int argc, char** argv, char** col_name);

class Database {
  public:
    Database(const std::string& db_name);

    int insert_job(const std::string& payload, int client_id);

    Job get_job_by_id(int job_id);
    std::vector<Job> get_job_by_client_id(int client_id);

    std::vector<Job> get_all_jobs();
    std::vector<Job> get_jobs_by_status(const std::string& status);
};
