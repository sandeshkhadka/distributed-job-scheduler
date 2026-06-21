#include "pg_db.hpp"
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

PgDatabase* PgDatabase::_instance = nullptr;
std::mutex PgDatabase::_init_mutex;

PgDatabase::PgDatabase() {
    _conn = PQconnectdb("");
    if (PQstatus(_conn) != CONNECTION_OK) {
        std::string err = PQerrorMessage(_conn);
        PQfinish(_conn);
        throw std::runtime_error("PostgreSQL connection failed: " + err);
    }
}

PgDatabase::~PgDatabase() {
    if (_conn)
        PQfinish(_conn);
}

void PgDatabase::execute(const std::string& query,
                         int (*callback)(void*, int, char**, char**),
                         void* container) {
    std::lock_guard<std::mutex> lock(_mutex);
    PGresult* res = PQexec(_conn, query.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::string err = PQerrorMessage(_conn);
        PQclear(res);
        throw std::runtime_error(err);
    }
    if (callback && PQresultStatus(res) == PGRES_TUPLES_OK) {
        int nrows = PQntuples(res);
        int ncols = PQnfields(res);
        for (int i = 0; i < nrows; i++) {
            std::vector<char*> argv(ncols);
            std::vector<char*> col_names(ncols);
            std::vector<std::string> col_name_strs(ncols);
            for (int j = 0; j < ncols; j++) {
                if (PQgetisnull(res, i, j)) {
                    argv[j] = nullptr;
                } else {
                    argv[j] = PQgetvalue(res, i, j);
                }
                col_name_strs[j] = PQfname(res, j);
                col_names[j] = col_name_strs[j].data();
            }
            callback(container, ncols, argv.data(), col_names.data());
        }
    }
    PQclear(res);
}

void PgDatabase::execute(const std::string& query) { execute(query, nullptr, nullptr); }

int64_t PgDatabase::execute_insert(const std::string& query, const std::string& column) {
    std::lock_guard<std::mutex> lock(_mutex);
    PGresult* res = PQexec(_conn, query.c_str());
    ExecStatusType status = PQresultStatus(res);
    if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
        std::string err = PQerrorMessage(_conn);
        PQclear(res);
        throw std::runtime_error(err);
    }
    int64_t id = -1;
    if (status == PGRES_TUPLES_OK && PQntuples(res) > 0 && !PQgetisnull(res, 0, 0)) {
        id = std::stoll(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    return id;
}

void PgDatabase::apply_migrations(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open migrations file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    std::string stmt;
    for (char ch : content) {
        stmt += ch;
        if (ch == ';') {
            std::string trimmed = stmt;
            stmt.clear();
            size_t start = trimmed.find_first_not_of(" \t\n\r");
            if (start == std::string::npos)
                continue;
            size_t end = trimmed.find_last_not_of(" \t\n\r;");
            trimmed = trimmed.substr(start, end - start + 1);
            if (trimmed.empty())
                continue;
            execute(trimmed);
        }
    }
}
