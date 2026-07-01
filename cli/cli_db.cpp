#include "cli_db.hpp"
#include "database.h"
#include "sqlite_db.hpp"

CliDatabase::CliDatabase() : Database("cli.db") {
    // create client table
    std::string create_clients = "CREATE TABLE IF NOT EXISTS self_info ("
                                 "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                 "client_id INTEGER NOT NULL, "
                                 "name TEXT NOT NULL"
                                 ");";
    SqliteDatabase::instance().execute(create_clients);

    std::string create_auth = "CREATE TABLE IF NOT EXISTS auth ("
                              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                              "token TEXT NOT NULL"
                              ");";
    SqliteDatabase::instance().execute(create_auth);

    std::string create_cached_jobs = "CREATE TABLE IF NOT EXISTS cached_jobs ("
                                     "job_id INTEGER PRIMARY KEY, "
                                     "job_type TEXT NOT NULL, "
                                     "status TEXT NOT NULL, "
                                     "created_at TEXT NOT NULL, "
                                     "cached_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                                     ");";
    SqliteDatabase::instance().execute(create_cached_jobs);

    std::string create_cached_results = "CREATE TABLE IF NOT EXISTS cached_job_results ("
                                        "job_id INTEGER PRIMARY KEY, "
                                        "status TEXT NOT NULL, "
                                        "has_result INTEGER NOT NULL DEFAULT 0, "
                                        "success INTEGER, "
                                        "result_message TEXT, "
                                        "artifact_url TEXT, "
                                        "completed_at TEXT, "
                                        "cached_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                                        ");";
    SqliteDatabase::instance().execute(create_cached_results);
}

int CliDatabase::insert_client(const std::string& name, int client_id) {
    std::string insert = "INSERT INTO self_info (name, client_id) VALUES ('" + name + "', " +
                         std::to_string(client_id) + ");";
    SqliteDatabase::instance().execute(insert);
    return SqliteDatabase::instance().last_insert_rowid();
}

int CliDatabase::get_active_client() {
    std::string select = "SELECT client_id FROM self_info ORDER BY id DESC LIMIT 1;";
    int id = -1;
    SqliteDatabase::instance().execute(select, get_active_client_callback, &id);
    return id;
}

void CliDatabase::save_token(const std::string& token) {
    std::string insert = "INSERT INTO auth (token) VALUES ('" + token + "');";
    SqliteDatabase::instance().execute(insert);
}

std::string CliDatabase::get_token() {
    std::string select = "SELECT token FROM auth ORDER BY id DESC LIMIT 1;";
    std::string token;
    SqliteDatabase::instance().execute(select, get_token_callback, &token);
    return token;
}

void CliDatabase::cache_jobs(const std::vector<CachedJobEntry>& jobs) {
    SqliteDatabase::instance().execute("BEGIN");
    for (const auto& j : jobs) {
        std::string q = "INSERT OR REPLACE INTO cached_jobs "
                        "(job_id, job_type, status, created_at, cached_at) VALUES (" +
                        std::to_string(j.job_id) + ", '" + j.job_type + "', '" + j.status + "', '" +
                        j.created_at + "', CURRENT_TIMESTAMP)";
        SqliteDatabase::instance().execute(q);
    }
    SqliteDatabase::instance().execute("COMMIT");
}

std::vector<CachedJobEntry> CliDatabase::get_cached_jobs() {
    std::string q = "SELECT job_id, job_type, status, created_at, cached_at "
                    "FROM cached_jobs ORDER BY created_at DESC";
    std::vector<CachedJobEntry> jobs;
    SqliteDatabase::instance().execute(q, cached_jobs_cb, &jobs);
    return jobs;
}

void CliDatabase::clear_jobs_cache() {
    SqliteDatabase::instance().execute("DELETE FROM cached_jobs");
}

void CliDatabase::cache_job_result(int job_id,
                                   const std::string& status,
                                   bool has_result,
                                   bool success,
                                   const std::string& result_message,
                                   const std::string& artifact_url,
                                   const std::string& completed_at) {
    std::string q = "INSERT OR REPLACE INTO cached_job_results "
                    "(job_id, status, has_result, success, result_message, "
                    "artifact_url, completed_at, cached_at) VALUES (" +
                    std::to_string(job_id) + ", '" + status + "', " +
                    std::to_string(has_result ? 1 : 0) + ", " + std::to_string(success ? 1 : 0) +
                    ", '" + result_message + "', '" + artifact_url + "', '" + completed_at +
                    "', CURRENT_TIMESTAMP)";
    SqliteDatabase::instance().execute(q);
}

CachedJobResultEntry CliDatabase::get_cached_job_result(int job_id) {
    std::string q = "SELECT job_id, status, has_result, success, "
                    "result_message, artifact_url, completed_at, cached_at "
                    "FROM cached_job_results WHERE job_id = " +
                    std::to_string(job_id);
    CachedJobResultEntry entry{};
    SqliteDatabase::instance().execute(q, cached_job_result_cb, &entry);
    return entry;
}

void CliDatabase::clear_result_cache(int job_id) {
    std::string q = "DELETE FROM cached_job_results WHERE job_id = " + std::to_string(job_id);
    SqliteDatabase::instance().execute(q);
}

int get_active_client_callback(void* data, int, char** values, char**) {
    int* id = static_cast<int*>(data);
    *id = atoi(values[0]);
    return 0;
}

int get_token_callback(void* data, int, char** values, char**) {
    std::string* token = static_cast<std::string*>(data);
    if (values[0])
        *token = values[0];
    return 0;
}

int cached_jobs_cb(void* data, int argc, char** values, char**) {
    auto* jobs = static_cast<std::vector<CachedJobEntry>*>(data);
    CachedJobEntry e;
    if (argc >= 1 && values[0])
        e.job_id = std::stoi(values[0]);
    if (argc >= 2 && values[1])
        e.job_type = values[1];
    if (argc >= 3 && values[2])
        e.status = values[2];
    if (argc >= 4 && values[3])
        e.created_at = values[3];
    if (argc >= 5 && values[4])
        e.cached_at = values[4];
    jobs->push_back(e);
    return 0;
}

int cached_job_result_cb(void* data, int argc, char** values, char**) {
    auto* e = static_cast<CachedJobResultEntry*>(data);
    if (argc >= 1 && values[0])
        e->job_id = std::stoi(values[0]);
    if (argc >= 2 && values[1])
        e->status = values[1];
    if (argc >= 3 && values[2])
        e->has_result = std::stoi(values[2]) != 0;
    if (argc >= 4 && values[3])
        e->success = std::stoi(values[3]) != 0;
    if (argc >= 5 && values[4])
        e->result_message = values[4] ? values[4] : "";
    if (argc >= 6 && values[5])
        e->artifact_url = values[5] ? values[5] : "";
    if (argc >= 7 && values[6])
        e->completed_at = values[6] ? values[6] : "";
    if (argc >= 8 && values[7])
        e->cached_at = values[7] ? values[7] : "";
    return 0;
}
