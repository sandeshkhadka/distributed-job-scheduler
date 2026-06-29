#include "ebpf_metric_store.hpp"

void EbpfMetricStore::add_snapshot(int job_id, const djs::JobEbpfMetrics& snap) {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_[job_id].push_back(snap);
}

std::vector<djs::JobEbpfMetrics> EbpfMetricStore::drain(int job_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = metrics_.find(job_id);
    if (it == metrics_.end())
        return {};
    auto result = std::move(it->second);
    metrics_.erase(it);
    return result;
}

std::vector<int> EbpfMetricStore::active_job_ids() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<int> ids;
    ids.reserve(metrics_.size());
    for (const auto& [id, _] : metrics_) {
        ids.push_back(id);
    }
    return ids;
}
