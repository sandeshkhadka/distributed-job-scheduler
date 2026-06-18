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

int get_token_cb(void* data, int argc, char** argv, char** col_name) {
    std::string* token = static_cast<std::string*>(data);
    if (argv[0])
        *token = argv[0];
    return 0;
}
