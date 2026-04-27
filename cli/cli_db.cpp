#include "cli_db.hpp"
#include "database.h"
#include "sqlite_db.hpp"

CliDatabase::CliDatabase() : Database("cli.db") {
    // create client table
    std::string create_clients = "CREATE TABLE IF NOT EXISTS self_info ("
                                 "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                 "client_id INTEGER NOT NULL, " // from the
                                                                // scheduler
                                 "name TEXT NOT NULL"
                                 ");";
    SqliteDatabase::instance().execute(create_clients);
}

int CliDatabase::insert_client(const Client& client) {}

int CliDatabase::get_active_client() {}
