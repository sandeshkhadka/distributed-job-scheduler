#include "selectors/fcfs_selector.hpp"

std::string FCFSSelector::name() const { return "fcfs"; }

Job FCFSSelector::select_job(const Worker& worker,
                             const std::vector<Job>& jobs,
                             SchedulerDatabase& db) {
    const Job* oldest = nullptr;
    for (const auto& job : jobs) {
        if (job.status != "not started")
            continue;
        if (!oldest || job.created_at < oldest->created_at)
            oldest = &job;
    }
    if (oldest)
        return *oldest;
    return Job{};
}
