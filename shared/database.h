#include <string>
#include <vector>
struct Job {
    int id;
    std::string payload;
};

class Database {
  public:
    Database(const std::string& db_name);

    Job get_job_by_id(int job_id);

    std::vector<Job> get_all_jobs();
    std::vector<Job> get_jobs_by_status(const std::string& status);
};
