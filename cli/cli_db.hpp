#pragma once
#include "database.h"
#include <string>
#include <vector>

struct Client {
    int client_id;
    std::string name;
};

struct CachedJobEntry {
    int job_id;
    std::string job_type;
    std::string status;
    std::string created_at;
    std::string cached_at;
};

struct CachedJobResultEntry {
    int job_id;
    std::string status;
    bool has_result;
    bool success;
    std::string result_message;
    std::string artifact_url;
    std::string completed_at;
    std::string cached_at;
};

int get_active_client_callback(void* data, int, char** values, char**);
int get_token_callback(void* data, int, char** values, char**);
int cached_jobs_cb(void* data, int, char** values, char**);
int cached_job_result_cb(void* data, int, char** values, char**);

class CliDatabase : public Database {
  public:
    CliDatabase();

    int insert_client(const std::string& name, int client_id);
    int get_active_client();

    void save_token(const std::string& token);
    std::string get_token();

    // cache methods
    void cache_jobs(const std::vector<CachedJobEntry>& jobs);
    std::vector<CachedJobEntry> get_cached_jobs();
    void clear_jobs_cache();

    void cache_job_result(int job_id,
                          const std::string& status,
                          bool has_result,
                          bool success,
                          const std::string& result_message,
                          const std::string& artifact_url,
                          const std::string& completed_at);
    CachedJobResultEntry get_cached_job_result(int job_id);
    void clear_result_cache(int job_id);
};
