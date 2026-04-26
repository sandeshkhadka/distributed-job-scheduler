#include <string>

struct Job {
    int id;
    std::string payload;
};
class SchedulerDatabase {
  public:
    SchedulerDatabase();

    void insert_job(const std::string& payload);

    Job get_jobs_by_id(int id);
};
