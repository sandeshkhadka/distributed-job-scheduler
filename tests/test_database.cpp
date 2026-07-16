#include "logger.h"
#include "sqlite_db.hpp"
#include "gtest/gtest.h"
#include <cstring>

using Logger = DJS::Logger;

// SqliteDatabase is a singleton. We initialize it once with :memory:
// and manually create the schema, since SchedulerDatabase's constructor
// would try to re-initialize the singleton with "scheduler.db".
class DatabaseTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() {
        SqliteDatabase::init(":memory:");

        auto& db = SqliteDatabase::instance();
        db.execute("CREATE TABLE IF NOT EXISTS jobs ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "job_type TEXT NOT NULL, "
                   "params TEXT NOT NULL DEFAULT '', "
                   "status TEXT NOT NULL DEFAULT 'not started', "
                   "client_id INTEGER NOT NULL, "
                   "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, "
                   "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                   ");");
        db.execute("CREATE TABLE IF NOT EXISTS workers ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "cpu_cores INTEGER NOT NULL, "
                   "mem_size REAL NOT NULL, "
                   "disk_size REAL NOT NULL, "
                   "cpu_freq REAL NOT NULL, "
                   "os TEXT NOT NULL, "
                   "name TEXT NOT NULL, "
                   "status TEXT NOT NULL"
                   ");");
        db.execute("CREATE TABLE IF NOT EXISTS auth_tokens ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "token TEXT NOT NULL UNIQUE, "
                   "description TEXT, "
                   "token_type TEXT NOT NULL DEFAULT 'client' "
                   "  CHECK (token_type IN ('client', 'worker')), "
                   "active INTEGER NOT NULL DEFAULT 1, "
                   "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                   ");");
    }
};

// ---- Non-capturing callback helpers ----

struct JobRow {
    int id;
    char type[64];
    char status[16];
};
static int job_cb(void* data, int argc, char** argv, char**) {
    auto* row = static_cast<JobRow*>(data);
    if (argc >= 4 && argv[0] && argv[1] && argv[3]) {
        row->id = std::stoi(argv[0]);
        std::strncpy(row->type, argv[1], sizeof(row->type) - 1);
        std::strncpy(row->status, argv[3], sizeof(row->status) - 1);
    }
    return 0;
}

struct CountResult {
    int count[2];
}; // [0]=not_started, [1]=completed
static int count_cb(void* data, int argc, char** argv, char**) {
    auto* r = static_cast<CountResult*>(data);
    if (argc >= 2 && argv[0] && argv[1]) {
        if (std::strcmp(argv[1], "not started") == 0)
            r->count[0]++;
        if (std::strcmp(argv[1], "completed") == 0)
            r->count[1]++;
    }
    return 0;
}

struct WorkerRow {
    int id;
    int cores;
    char name[64];
};
static int worker_cb(void* data, int argc, char** argv, char**) {
    auto* w = static_cast<WorkerRow*>(data);
    if (argc >= 7 && argv[0] && argv[1] && argv[6]) {
        w->id = std::stoi(argv[0]);
        w->cores = std::stoi(argv[1]);
        std::strncpy(w->name, argv[6], sizeof(w->name) - 1);
    }
    return 0;
}

static int int_cb(void* data, int argc, char** argv, char**) {
    auto* val = static_cast<int*>(data);
    if (argc >= 1 && argv[0])
        *val = std::stoi(argv[0]);
    return 0;
}

// ---- Tests ----

TEST_F(DatabaseTest, InsertAndRetrieveJob) {
    auto& db = SqliteDatabase::instance();

    db.execute("INSERT INTO jobs (job_type, params, client_id) VALUES ('stress_cpu', '{}', 1)");
    int job_id = db.last_insert_rowid();
    EXPECT_GT(job_id, 0);

    JobRow row{};
    db.execute("SELECT id, job_type, params, status FROM jobs WHERE id = " + std::to_string(job_id),
               job_cb,
               &row);

    EXPECT_EQ(row.id, job_id);
    EXPECT_STREQ(row.type, "stress_cpu");
    EXPECT_STREQ(row.status, "not started");
}

TEST_F(DatabaseTest, QueryJobsByStatus) {
    auto& db = SqliteDatabase::instance();

    db.execute("INSERT INTO jobs (job_type, params, status, client_id) VALUES ('cpu', '{}', 'not "
               "started', 1)");
    db.execute("INSERT INTO jobs (job_type, params, status, client_id) VALUES ('mem', '{}', "
               "'completed', 1)");

    CountResult counts{};
    db.execute("SELECT id, status FROM jobs", count_cb, &counts);

    EXPECT_GE(counts.count[0], 1); // at least 1 not started
    EXPECT_GE(counts.count[1], 1); // at least 1 completed
}

TEST_F(DatabaseTest, InsertAndQueryWorker) {
    auto& db = SqliteDatabase::instance();

    db.execute("INSERT INTO workers (cpu_cores, mem_size, disk_size, cpu_freq, os, name, status) "
               "VALUES (8, 16384, 512000, 3.2, 'Linux', 'worker-1', 'active')");
    int worker_id = db.last_insert_rowid();
    EXPECT_GT(worker_id, 0);

    WorkerRow w{};
    db.execute("SELECT id, cpu_cores, mem_size, disk_size, cpu_freq, os, name, status "
               "FROM workers WHERE id = " +
                   std::to_string(worker_id),
               worker_cb,
               &w);

    EXPECT_EQ(w.id, worker_id);
    EXPECT_EQ(w.cores, 8);
    EXPECT_STREQ(w.name, "worker-1");
}

TEST_F(DatabaseTest, TokenValidation) {
    auto& db = SqliteDatabase::instance();

    db.execute("INSERT INTO auth_tokens (token, token_type) VALUES ('valid-token-1', 'client')");
    db.execute("INSERT INTO auth_tokens (token, token_type) VALUES ('valid-token-2', 'worker')");

    auto check = [&](const std::string& token, const std::string& type) -> int {
        std::string q = "SELECT COUNT(*) FROM auth_tokens WHERE token = '" + token +
                        "' AND token_type = '" + type + "' AND active = 1";
        int count = 0;
        db.execute(q, int_cb, &count);
        return count;
    };

    EXPECT_EQ(check("valid-token-1", "client"), 1);
    EXPECT_EQ(check("valid-token-2", "worker"), 1);
    EXPECT_EQ(check("valid-token-1", "worker"), 0); // wrong type
    EXPECT_EQ(check("nonexistent", "client"), 0);   // doesn't exist
}
