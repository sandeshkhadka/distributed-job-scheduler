#pragma once
#include <libpq-fe.h>
#include <mutex>
#include <stdexcept>
#include <string>

class PgDatabase {
  private:
    static std::mutex _init_mutex;
    static PgDatabase* _instance;
    PGconn* _conn;
    std::mutex _mutex;

  public:
    void
    execute(const std::string& query, int (*callback)(void*, int, char**, char**), void* container);
    void execute(const std::string& query);
    int64_t execute_insert(const std::string& query, const std::string& column = "id");

    static void init() {
        std::lock_guard<std::mutex> lock(_init_mutex);
        if (_instance)
            return;
        _instance = new PgDatabase();
    }
    static PgDatabase& instance() {
        if (!_instance)
            throw std::runtime_error("Database not initialized");
        return *_instance;
    }

    void apply_migrations(const std::string& path);

  private:
    PgDatabase();
    ~PgDatabase();
};
