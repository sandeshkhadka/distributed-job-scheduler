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
