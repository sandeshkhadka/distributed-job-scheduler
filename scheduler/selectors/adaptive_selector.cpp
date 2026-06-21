#include "selectors/adaptive_selector.hpp"
#include <string>

namespace {

const Job* pick_oldest(const std::vector<Job>& jobs) {
    const Job* oldest = nullptr;
    for (const auto& job : jobs) {
        if (job.status != "not started")
            continue;
        if (!oldest || job.created_at < oldest->created_at)
            oldest = &job;
    }
    return oldest;
}

} // anonymous namespace

std::string AdaptiveSelector::name() const { return "adaptive"; }

Job AdaptiveSelector::select_job(const Worker& worker,
                                 const std::vector<Job>& jobs,
                                 SchedulerDatabase& db) {
    auto metrics = db.get_worker_latest_metrics(worker.id);
    if (!metrics.has_value())
        return Job{};

    auto spikes = db.get_avg_spikes();

    const Job* safe_known = nullptr;
    const Job* unknown = nullptr;

    for (const auto& job : jobs) {
        if (job.status != "not started")
            continue;

        auto it = spikes.find(job.job_type);
        if (it == spikes.end()) {
            if (!unknown || job.created_at < unknown->created_at)
                unknown = &job;
            continue;
        }

        double pred_cpu = metrics->cpu_percent + it->second.avg_cpu_spike;
        double spike_mb = (it->second.avg_memory_spike / 100.0) * metrics->memory_total_mb;
        // 200MB buffer gives the worker breathing room
        double pred_mem = metrics->memory_used_mb + spike_mb + 200.0;

        if (pred_cpu <= 100.0 && pred_mem <= metrics->memory_total_mb) {
            if (!safe_known || job.created_at < safe_known->created_at)
                safe_known = &job;
        }
    }

    if (safe_known)
        return *safe_known;

    // Unknown types need bootstrapping
    if (unknown)
        return *unknown;

    return Job{};
}
