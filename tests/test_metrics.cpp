#include "metrics_collector.hpp"
#include "gtest/gtest.h"

TEST(MetricsCollectorTest, CollectReturnsNonDefaultValues) {
    MetricsCollector collector;
    WorkerMetricsData data = collector.collect();

    // On a Linux system these should be > 0
    EXPECT_GT(data.memory_total_mb, 0.0);
    EXPECT_GT(data.memory_percent, 0.0);

    // CPU should be between 0 and 100
    EXPECT_GE(data.cpu_percent, 0.0);
    EXPECT_LE(data.cpu_percent, 100.0);

    // Disk space should be reasonable
    EXPECT_GT(data.disk_total_mb, 0.0);
    EXPECT_GE(data.disk_percent, 0.0);
}
