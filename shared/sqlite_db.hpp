#include <functional>
#include <mutex>
#include <sqlite3.h>
#include <stdexcept>
#include <string>
class SqliteDatabase {
  private:
    static std::mutex _init_mutex;
    static SqliteDatabase* _instance;
    sqlite3* _db;
    std::mutex _mutex;
    std::string _db_name;

  public:
    void execute(const std::string& query, int (*callback)(void*, int, char**, char**), void*);
    void execute(const std::string& query);

    static void init(const std::string& db_path) {
        std::lock_guard<std::mutex> lock(_init_mutex);
        if (_instance) {
            throw std::runtime_error("Database already initalized");
        }
        _instance = new SqliteDatabase(db_path);
    }
    static SqliteDatabase& instance() {
        if (!_instance) {
            throw new std::runtime_error("Database not initalized");
        }
        return *_instance;
    }

  private:
    SqliteDatabase(const std::string& db_path);
    ~SqliteDatabase();
};
