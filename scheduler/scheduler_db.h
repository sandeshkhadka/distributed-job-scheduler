#include <string>
class SchedulerDatabase {
  public:
    SchedulerDatabase();

    void insert_job(const std::string& payload);
};
