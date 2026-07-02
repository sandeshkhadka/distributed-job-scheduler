#include "ebpf_metric_store.hpp"
#include "scheduler.pb.h"
#include "gtest/gtest.h"

class EbpfMetricStoreTest : public ::testing::Test {
  protected:
    EbpfMetricStore store;

    djs::JobEbpfMetrics make_snapshot(int job_id, int64_t cpu_us, int64_t mem_bytes) {
        djs::JobEbpfMetrics snap;
        snap.set_job_id(job_id);
        snap.set_cpu_usage_us(cpu_us);
        snap.set_mem_current_bytes(mem_bytes);
        snap.set_syscall_read_count(100);
        snap.set_syscall_write_count(50);
        snap.set_io_read_bytes(4096);
        snap.set_io_write_bytes(2048);
        snap.set_net_rx_bytes(1024);
        snap.set_net_tx_bytes(512);
        return snap;
    }
};

TEST_F(EbpfMetricStoreTest, AddAndDrain) {
    store.add_snapshot(42, make_snapshot(42, 1000000, 67108864));

    auto result = store.drain(42);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].job_id(), 42);
    EXPECT_EQ(result[0].cpu_usage_us(), 1000000);
    EXPECT_EQ(result[0].mem_current_bytes(), 67108864);
    EXPECT_EQ(result[0].io_read_bytes(), 4096);
}

TEST_F(EbpfMetricStoreTest, DrainRemovesData) {
    store.add_snapshot(1, make_snapshot(1, 500000, 33554432));

    auto first = store.drain(1);
    ASSERT_EQ(first.size(), 1);

    auto second = store.drain(1);
    EXPECT_TRUE(second.empty());
}

TEST_F(EbpfMetricStoreTest, ActiveJobIds) {
    store.add_snapshot(10, make_snapshot(10, 100, 100));
    store.add_snapshot(20, make_snapshot(20, 200, 200));
    store.add_snapshot(30, make_snapshot(30, 300, 300));

    auto ids = store.active_job_ids();
    EXPECT_EQ(ids.size(), 3);

    store.drain(20);
    ids = store.active_job_ids();
    EXPECT_EQ(ids.size(), 2);
}

TEST_F(EbpfMetricStoreTest, HandlesMultipleSnapshots) {
    store.add_snapshot(1, make_snapshot(1, 100, 100));
    store.add_snapshot(1, make_snapshot(1, 200, 200));
    store.add_snapshot(1, make_snapshot(1, 300, 300));

    auto result = store.drain(1);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[2].cpu_usage_us(), 300);
}

TEST_F(EbpfMetricStoreTest, DrainNonexistentReturnsEmpty) {
    auto result = store.drain(999);
    EXPECT_TRUE(result.empty());
}
