#include <mutex>
#include <sqlite3.h>
#include <stdexcept>
#include <string>
class Database {
  private:
    static std::mutex _init_mutex;
    static Database* _instance;
    sqlite3* _db;
    std::mutex _mutex;
    std::string _db_name;

  public:
    void execute(const std::string& query);

    static void init(const std::string& db_path) {
        std::lock_guard<std::mutex> lock(_init_mutex);
        if (_instance) {
            throw std::runtime_error("Database already initalized");
        }
        _instance = new Database(db_path);
    }
    static Database& instance() {
        if (!_instance) {
            throw new std::runtime_error("Database not initalized");
        }
        return *_instance;
    }

  private:
    Database(const std::string& db_path);
    ~Database();
};
