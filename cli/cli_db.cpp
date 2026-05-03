#include "cli_db.hpp"
#include "database.h"
#include "sqlite_db.hpp"
#include <string>

CliDatabase::CliDatabase() : Database("cli.db") {}

int CliDatabase::insert_client(const Client& client) {
    std::string query = "INSERT INTO clients (id, name, status) VALUES (" +
                        std::to_string(client.id) + ", '" + client.name + "', '" + client.status +
                        "')";

    int id;
    SqliteDatabase::instance().execute(query);
    id = SqliteDatabase::instance().last_insert_rowid();
    return id;
}
