#include "database.h"
#include "mock_db.hpp"
#include "selectors/adaptive_selector.hpp"
#include "selectors/fcfs_selector.hpp"
#include "selectors/sjf_selector.hpp"
#include "gtest/gtest.h"
#include <vector>

// ============================================================
// FCFSSelector Tests
// ============================================================

TEST(FCFSSelectorTest, ReturnsEmptyJobWhenNoJobs) {
    FCFSSelector selector;
    MockDatabase db;
    Worker worker;
    std::vector<Job> jobs;

    Job result = selector.select_job(worker, jobs, db);
    EXPECT_EQ(result.id, 0);
    EXPECT_TRUE(result.job_type.empty());
}

TEST(FCFSSelectorTest, PicksOldestJob) {
    FCFSSelector selector;
    MockDatabase db;
    Worker worker;

    Job j1;
    j1.id = 1;
    j1.status = "not started";
    j1.created_at = "2025-07-01 12:00:00";
    Job j2;
    j2.id = 2;
    j2.status = "not started";
    j2.created_at = "2025-07-01 11:00:00";
    Job j3;
    j3.id = 3;
    j3.status = "not started";
    j3.created_at = "2025-07-01 13:00:00";

    std::vector<Job> jobs = {j1, j2, j3};
    Job result = selector.select_job(worker, jobs, db);

    EXPECT_EQ(result.id, 2);
}

TEST(FCFSSelectorTest, SkipsNonStartedJobs) {
    FCFSSelector selector;
    MockDatabase db;
    Worker worker;

    Job j1;
    j1.id = 1;
    j1.status = "completed";
    j1.created_at = "2025-07-01 10:00:00";
    Job j2;
    j2.id = 2;
    j2.status = "not started";
    j2.created_at = "2025-07-01 12:00:00";
    Job j3;
    j3.id = 3;
    j3.status = "ongoing";
    j3.created_at = "2025-07-01 11:00:00";

    std::vector<Job> jobs = {j1, j2, j3};
    Job result = selector.select_job(worker, jobs, db);

    EXPECT_EQ(result.id, 2);
}

// ============================================================
// SJFSelector Tests
// ============================================================

TEST(SJFSelectorTest, PicksJobWithShortestDuration) {
    SJFSelector selector;
    MockDatabase db;
    Worker worker;

    db.durations["stress_mem"] = 2000.0;
    db.durations["stress_cpu"] = 5000.0;
    db.durations["stress_io"] = 8000.0;

    Job cpu;
    cpu.id = 1;
    cpu.job_type = "stress_cpu";
    cpu.status = "not started";
    cpu.created_at = "2025-07-01 12:00:00";
    Job mem;
    mem.id = 2;
    mem.job_type = "stress_mem";
    mem.status = "not started";
    mem.created_at = "2025-07-01 11:00:00";
    Job io;
    io.id = 3;
    io.job_type = "stress_io";
    io.status = "not started";
    io.created_at = "2025-07-01 13:00:00";

    std::vector<Job> jobs = {cpu, mem, io};
    Job result = selector.select_job(worker, jobs, db);

    EXPECT_EQ(result.id, 2);
    EXPECT_EQ(result.job_type, "stress_mem");
}

TEST(SJFSelectorTest, IgnoresJobsWithLongerDuration) {
    SJFSelector selector;
    MockDatabase db;
    Worker worker;

    db.durations["stress_cpu"] = 1000.0;
    db.durations["stress_mem"] = 99999.0;

    Job cpu;
    cpu.id = 1;
    cpu.job_type = "stress_cpu";
    cpu.status = "not started";
    cpu.created_at = "2025-07-01 12:00:00";
    Job mem;
    mem.id = 2;
    mem.job_type = "stress_mem";
    mem.status = "not started";
    mem.created_at = "2025-07-01 11:00:00";

    std::vector<Job> jobs = {cpu, mem};
    Job result = selector.select_job(worker, jobs, db);

    EXPECT_EQ(result.id, 1);
    EXPECT_EQ(result.job_type, "stress_cpu");
}

TEST(SJFSelectorTest, FallsBackToFCFSWhenNoDurations) {
    SJFSelector selector;
    MockDatabase db;
    Worker worker;

    Job j1;
    j1.id = 1;
    j1.job_type = "stress_cpu";
    j1.status = "not started";
    j1.created_at = "2025-07-01 13:00:00";
    Job j2;
    j2.id = 2;
    j2.job_type = "stress_mem";
    j2.status = "not started";
    j2.created_at = "2025-07-01 11:00:00";
    Job j3;
    j3.id = 3;
    j3.job_type = "stress_io";
    j3.status = "not started";
    j3.created_at = "2025-07-01 12:00:00";

    std::vector<Job> jobs = {j1, j2, j3};
    Job result = selector.select_job(worker, jobs, db);

    EXPECT_EQ(result.id, 2);
}

TEST(SJFSelectorTest, FallsBackWithOnlyUnknownTypes) {
    SJFSelector selector;
    MockDatabase db;
    Worker worker;

    db.durations["stress_cpu"] = 5000.0;

    Job io;
    io.id = 1;
    io.job_type = "stress_io";
    io.status = "not started";
    io.created_at = "2025-07-01 12:00:00";
    Job mem;
    mem.id = 2;
    mem.job_type = "stress_mem";
    mem.status = "not started";
    mem.created_at = "2025-07-01 11:00:00";

    std::vector<Job> jobs = {io, mem};
    Job result = selector.select_job(worker, jobs, db);

    EXPECT_EQ(result.id, 2);
}

// ============================================================
// AdaptiveSelector Tests
// ============================================================

TEST(AdaptiveSelectorTest, ReturnsEmptyWhenNoMetrics) {
    AdaptiveSelector selector;
    MockDatabase db;
    Worker worker;
    worker.id = 1;

    Job j1;
    j1.id = 1;
    j1.job_type = "stress_cpu";
    j1.status = "not started";
    std::vector<Job> jobs = {j1};

    Job result = selector.select_job(worker, jobs, db);
    EXPECT_EQ(result.id, 0);
}

TEST(AdaptiveSelectorTest, PicksSafeKnownJob) {
    AdaptiveSelector selector;
    MockDatabase db;
    Worker worker;
    worker.id = 1;

    db.metrics = WorkerMetricsBrief{50.0, 30.0, 2048.0, 8192.0};
    db.spikes["stress_cpu"] = JobTypeSpikes{20.0, 5.0, 5};

    Job cpu;
    cpu.id = 1;
    cpu.job_type = "stress_cpu";
    cpu.status = "not started";
    cpu.created_at = "2025-07-01 12:00:00";
    std::vector<Job> jobs = {cpu};

    Job result = selector.select_job(worker, jobs, db);
    EXPECT_EQ(result.id, 1);
}

TEST(AdaptiveSelectorTest, PicksUnknownWhenNoSafeKnown) {
    AdaptiveSelector selector;
    MockDatabase db;
    Worker worker;
    worker.id = 1;

    db.metrics = WorkerMetricsBrief{50.0, 30.0, 2048.0, 8192.0};
    // No spikes data means all jobs are "unknown"

    Job uk;
    uk.id = 1;
    uk.job_type = "stress_foo";
    uk.status = "not started";
    uk.created_at = "2025-07-01 12:00:00";
    std::vector<Job> jobs = {uk};

    Job result = selector.select_job(worker, jobs, db);
    EXPECT_EQ(result.id, 1);
}

TEST(AdaptiveSelectorTest, RejectsJobThatWouldOverloadCPU) {
    AdaptiveSelector selector;
    MockDatabase db;
    Worker worker;
    worker.id = 1;

    // At 95% CPU, a job with a 10% spike would exceed 100%
    db.metrics = WorkerMetricsBrief{95.0, 30.0, 2048.0, 8192.0};
    db.spikes["stress_cpu"] = JobTypeSpikes{10.0, 5.0, 5};

    Job cpu;
    cpu.id = 1;
    cpu.job_type = "stress_cpu";
    cpu.status = "not started";
    cpu.created_at = "2025-07-01 12:00:00";
    std::vector<Job> jobs = {cpu};

    Job result = selector.select_job(worker, jobs, db);
    EXPECT_EQ(result.id, 0);
}

TEST(AdaptiveSelectorTest, PicksOldestSafeAmongMultiple) {
    AdaptiveSelector selector;
    MockDatabase db;
    Worker worker;
    worker.id = 1;

    db.metrics = WorkerMetricsBrief{30.0, 20.0, 1024.0, 8192.0};
    db.spikes["stress_cpu"] = JobTypeSpikes{10.0, 5.0, 5};
    db.spikes["stress_mem"] = JobTypeSpikes{5.0, 10.0, 5};

    Job cpu;
    cpu.id = 1;
    cpu.job_type = "stress_cpu";
    cpu.status = "not started";
    cpu.created_at = "2025-07-01 13:00:00";
    Job mem;
    mem.id = 2;
    mem.job_type = "stress_mem";
    mem.status = "not started";
    mem.created_at = "2025-07-01 11:00:00";

    std::vector<Job> jobs = {cpu, mem};
    Job result = selector.select_job(worker, jobs, db);

    EXPECT_EQ(result.id, 2);
    EXPECT_EQ(result.job_type, "stress_mem");
}

// ============================================================
// Cross-Selector Comparison Tests
// ============================================================

TEST(SelectorComparisonTest, AdaptivePreventsOverloadWhileFCFSandSJFDoNot) {
    Worker worker;
    worker.id = 1;

    // Worker at 80% CPU, 30% memory (2048 / 8192 MB)
    MockDatabase db;
    db.metrics = WorkerMetricsBrief{80.0, 30.0, 2048.0, 8192.0};

    // Job 1: stress_cpu — older, shorter runtime, high CPU spike (30%)
    // Job 2: stress_io  — newer, longer runtime, low CPU spike (5%)
    //
    // stress_cpu on this worker:  80 + 30 = 110% CPU → overload
    // stress_io  on this worker:  80 + 5  =  85% CPU → safe

    Job cpu;
    cpu.id = 1;
    cpu.job_type = "stress_cpu";
    cpu.status = "not started";
    cpu.created_at = "2025-07-01 11:00:00";

    Job io;
    io.id = 2;
    io.job_type = "stress_io";
    io.status = "not started";
    io.created_at = "2025-07-01 13:00:00";

    std::vector<Job> jobs = {cpu, io};

    db.durations["stress_cpu"] = 10000.0;
    db.durations["stress_io"] = 60000.0;
    db.spikes["stress_cpu"] = JobTypeSpikes{30.0, 5.0, 10};
    db.spikes["stress_io"] = JobTypeSpikes{5.0, 10.0, 10};

    // FCFS: picks oldest (stress_cpu) — would overload the worker
    {
        FCFSSelector selector;
        Job result = selector.select_job(worker, jobs, db);
        EXPECT_EQ(result.id, 1) << "FCFS picks oldest job regardless of load";
    }

    // SJF: picks shortest (stress_cpu) — would overload the worker
    {
        SJFSelector selector;
        Job result = selector.select_job(worker, jobs, db);
        EXPECT_EQ(result.id, 1) << "SJF picks shortest job regardless of load";
    }

    // Adaptive: rejects overload, picks safe (stress_io)
    {
        AdaptiveSelector selector;
        Job result = selector.select_job(worker, jobs, db);
        EXPECT_EQ(result.id, 2) << "Adaptive rejects overload and picks the safe job";
    }
}
