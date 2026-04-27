#include "worker_db.hpp"
#include "sqlite_db.hpp"
#include <string>

WorkerDatabase::WorkerDatabase() : Database("workers.db") {
    // create client table
    std::string create_active_workers = "CREATE TABLE IF NOT EXISTS active_workers ("
                                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                        "server_id INTEGER NOT NULL, "
                                        "name TEXT NOT NULL"
                                        ");";
    SqliteDatabase::instance().execute(create_active_workers);
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
    *worker_id = atoi(argv[1]); // 0 => id, 1 => server_id of the worker
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
