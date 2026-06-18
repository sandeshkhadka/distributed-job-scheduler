#include "worker_db.hpp"
#include "sqlite_db.hpp"
#include <string>

WorkerDatabase::WorkerDatabase() : Database("workers.db") {
    std::string create_active_workers = "CREATE TABLE IF NOT EXISTS active_workers ("
                                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                        "server_id INTEGER NOT NULL, "
                                        "name TEXT NOT NULL"
                                        ");";
    SqliteDatabase::instance().execute(create_active_workers);

    std::string create_auth = "CREATE TABLE IF NOT EXISTS auth ("
                              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                              "token TEXT NOT NULL"
                              ");";
    SqliteDatabase::instance().execute(create_auth);

    std::string create_results = "CREATE TABLE IF NOT EXISTS job_results ("
                                 "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                 "job_id INTEGER NOT NULL, "
                                 "success INTEGER NOT NULL, "
                                 "message TEXT, "
                                 "artifact_url TEXT, "
                                 "posted INTEGER NOT NULL DEFAULT 0, "
                                 "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                                 ");";
    SqliteDatabase::instance().execute(create_results);
}

int WorkerDatabase::get_registered_worker_id() {
    std::string create_active_workers = "SELECT * FROM active_workers;";
    int worker_id{-1};
    SqliteDatabase::instance().execute(
        create_active_workers, get_registered_worker_id_cb, &worker_id);

    return worker_id;
}

int get_registered_worker_id_cb(void* data, int argc, char** argv, char** col_name) {
    int* worker_id = static_cast<int*>(data);
    *worker_id = atoi(argv[1]);
    return 0;
}

int WorkerDatabase::insert_worker(const Worker& worker) {
    std::string query = "INSERT INTO active_workers (server_id, name) VALUES (" +
                        std::to_string(worker.server_id) + ", '" + worker.name + "')";
    int id;
    SqliteDatabase::instance().execute(query);
    id = SqliteDatabase::instance().last_insert_rowid();
    return id;
}

void WorkerDatabase::save_token(const std::string& token) {
    std::string insert = "INSERT INTO auth (token) VALUES ('" + token + "');";
    SqliteDatabase::instance().execute(insert);
}

std::string WorkerDatabase::get_token() {
    std::string select = "SELECT token FROM auth ORDER BY id DESC LIMIT 1;";
    std::string token;
    SqliteDatabase::instance().execute(select, get_token_cb, &token);
    return token;
}

void WorkerDatabase::insert_job_result(int job_id,
                                       bool success,
                                       const std::string& message,
                                       const std::string& artifact_url) {
    std::string query =
        "INSERT INTO job_results (job_id, success, message, artifact_url) VALUES (" +
        std::to_string(job_id) + ", " + std::to_string(success ? 1 : 0) + ", '" + message + "', '" +
        artifact_url + "')";
    SqliteDatabase::instance().execute(query);
}

std::vector<JobResultRecord> WorkerDatabase::get_unposted_results() {
    std::vector<JobResultRecord> records;
    SqliteDatabase::instance().execute(
        "SELECT id, job_id, success, message, artifact_url, posted, created_at "
        "FROM job_results WHERE posted = 0 ORDER BY id ASC",
        job_result_list_cb,
        &records);
    return records;
}

void WorkerDatabase::mark_result_posted(int result_id) {
    std::string query = "UPDATE job_results SET posted = 1 WHERE id = " + std::to_string(result_id);
    SqliteDatabase::instance().execute(query);
}

int get_token_cb(void* data, int argc, char** argv, char** col_name) {
    std::string* token = static_cast<std::string*>(data);
    if (argv[0])
        *token = argv[0];
    return 0;
}

int job_result_list_cb(void* data, int argc, char** argv, char** col_name) {
    auto* records = static_cast<std::vector<JobResultRecord>*>(data);
    JobResultRecord r;
    r.id = atoi(argv[0]);
    r.job_id = atoi(argv[1]);
    r.success = atoi(argv[2]) != 0;
    if (argc > 3 && argv[3])
        r.message = argv[3];
    if (argc > 4 && argv[4])
        r.artifact_url = argv[4];
    r.posted = argc > 5 ? atoi(argv[5]) : 0;
    if (argc > 6 && argv[6])
        r.created_at = argv[6];
    records->push_back(r);
    return 0;
}
