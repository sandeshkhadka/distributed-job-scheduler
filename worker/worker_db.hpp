#pragma once
#include "database.h"
#include <string>
#include <vector>

struct Worker {
    int server_id;
    std::string name;
};

struct JobResultRecord {
    int id;
    int job_id;
    bool success;
    std::string message;
    std::string artifact_url;
    int posted;
    std::string created_at;
};

int get_registered_worker_id_cb(void* data, int argc, char** argv, char** col_name);
int get_token_cb(void* data, int argc, char** argv, char** col_name);
int job_result_list_cb(void* data, int argc, char** argv, char** col_name);

class WorkerDatabase : public Database {
  public:
    WorkerDatabase();

    int get_registered_worker_id();
    int insert_worker(const Worker& worker);

    void save_token(const std::string& token);
    std::string get_token();

    void insert_job_result(int job_id,
                           bool success,
                           const std::string& message,
                           const std::string& artifact_url);
    std::vector<JobResultRecord> get_unposted_results();
    void mark_result_posted(int result_id);
};
