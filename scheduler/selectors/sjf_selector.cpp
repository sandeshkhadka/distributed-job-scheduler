#include "selectors/sjf_selector.hpp"
#include <climits>
#include <map>
#include <string>

std::string SJFSelector::name() const { return "sjf"; }

Job SJFSelector::select_job(const Worker& worker,
                            const std::vector<Job>& jobs,
                            ISchedulerDatabase& db) {
    auto avg_durations = db.get_avg_durations();

    const Job* best = nullptr;
    double best_dur = static_cast<double>(LLONG_MAX);

    for (const auto& job : jobs) {
        if (job.status != "not started")
            continue;
        auto it = avg_durations.find(job.job_type);
        if (it != avg_durations.end() && it->second < best_dur) {
            best = &job;
            best_dur = it->second;
        }
    }

    if (!best) {
        const Job* oldest = nullptr;
        for (const auto& job : jobs) {
            if (job.status != "not started")
                continue;
            if (!oldest || job.created_at < oldest->created_at)
                oldest = &job;
        }
        if (oldest)
            return *oldest;
    }

    if (best)
        return *best;
    return Job{};
}
