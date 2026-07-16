# Setup

## Install Requirements

Fedora:
```bash
sudo dnf install clang cmake clang-tools-extra sqlite sqlite-devel

# setup grpc
sudo dnf install grpc-devel grpc-plugins protobuf-devel protobuf-compiler

```

Arch:
```bash
sudo pacman -S clang cmake clang-tools-extra llvm-libs

sudo pacman -S grpc protobuf
```

### Test dependencies

Google Test 1.15.2+ is required. Install it via your package manager or build
from source.

Fedora:
```bash
sudo dnf install gtest-devel
```

Arch:
```bash
sudo pacman -S gtest
```

## Setup

```
git clone git@github.com:sandeshkhadka/distributed-job-scheduler.git
cd distributed-job-scheduler
git config core.hooksPath .githooks
```

## Build

```
cmake -B build
cmake --build build
```

## Testing

Tests use Google Test and cover selectors (FCFS, SJF, Adaptive), database schema
operations, metric store, and metrics collection. Build tests separately with:

```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build --target run_tests
```

Run the test binary:

```bash
./build/tests/run_tests
```

The test suite includes 22 unit tests across 6 suites (3 selectors, database, metric store, metrics collector). All tests run in ~2 ms and do not require root or libbpf-devel.

## Usage

### 0. Start the scheduler

The scheduler must be running before any client or worker can connect.

```
./build/scheduler
```

Listens on `0.0.0.0:50051` (default, not configurable yet).

### 1. Admin - token management

All authentication uses bearer tokens stored in the scheduler database. Two token types exist: `client` (for the CLI) and `worker` (for worker nodes). A token of one type cannot be used for the other.

```
# Generate a client token
./build/admin gen-token --desc "my laptop" --type client

# Generate a worker token
./build/admin gen-token --desc "server-1" --type worker

# List all tokens
./build/admin list-tokens

# Revoke a token by ID
./build/admin revoke-token --id 2

# Show usage for a token
./build/admin usage --id 1
```

`--desc` is a human-readable label. It has no effect on behavior. `--type` controls authorization; client tokens can only submit jobs, worker tokens can only claim and report on jobs.

### 2. Client - register and submit jobs

Register with a client token once. The token is cached locally in `cli.db`.

```
./build/cli register --token <client_token_hex>
```

Submit jobs with `--type` and repeatable `--param`:

```
# stress_cpu: burn N cores for M milliseconds
./build/cli submit --type stress_cpu --param cores=2 --param duration_ms=5000

# stress_mem: allocate N MB for M milliseconds
./build/cli submit --type stress_mem --param mb=128 --param duration_ms=3000

# stress_io: write/read N MB temp file for M ms. Optional mode: read, write, rw
./build/cli submit --type stress_io --param mb=64 --param duration_ms=2000 --param mode=rw

# mixed_load: run cpu + memory + io stress concurrently
./build/cli submit --type mixed_load --param cpu_cores=2 --param mem_mb=64 --param io_mb=16 --param duration_ms=5000
```

List your submitted jobs (results cached locally; use `--refresh` to fetch fresh data):

```
./build/cli jobs
./build/cli jobs --refresh
```

Get a specific job's result:

```
./build/cli result --id 1
./build/cli result --id 1 --refresh
```

Job types and their parameters:

| Type | Params | Defaults | Behavior |
|---|---|---|---|
| `stress_cpu` | `cores`, `duration_ms` | 1, 5000 | Busy-loops on N cores with sqrt/sin/cos |
| `stress_mem` | `mb`, `duration_ms` | 64, 5000 | Allocates N MB in 1 MB chunks, touches all pages, holds for duration |
| `stress_io` | `mb`, `duration_ms`, `mode` | 16, 5000, rw | Writes/reads a temp file in /tmp. `mode` can be `read`, `write`, or `rw` |
| `mixed_load` | `cpu_cores`, `mem_mb`, `io_mb`, `duration_ms` | 1, 64, 16, 5000 | Runs CPU + memory + I/O stress in concurrent threads |

### 3. Worker - process jobs

The worker polls the scheduler for unassigned jobs and runs them in isolated child processes.

```
# First run (requires token)
./build/worker --token <worker_token_hex>

# Subsequent runs reuse the cached token
./build/worker

# Explicit path to djs-executor (auto-detected from argv[0] by default)
./build/worker --executor ./build/djs-executor
```

The token is cached in `workers.db` after the first successful registration. If the token was revoked on the scheduler, re-register with `--token <new_token>`.

The worker auto-detects the `djs-executor` binary relative to its own location. `./build/worker` looks for `./build/djs-executor`. Passing `--executor` overrides this.

### 4. Dashboard (experimental, requires PostgreSQL)

A web-based monitoring dashboard is available at `dashboard/`. It connects directly to the PostgreSQL database and provides read-only views of jobs, workers, and metrics.

```bash
cd dashboard
npm install
PORT=3000 node server.js
```

Open `http://localhost:3000` in a browser.

The dashboard shows an overview with summary statistics, a workers page with live metrics, a jobs page filterable by status, and detail pages for individual workers and jobs with eBPF metrics timeseries data. REST API endpoints are available at `/api/workers/:id/metrics`, `/api/workers/:id/jobs`, and `/api/jobs/:id/ebpf`.

Note: The dashboard requires a running PostgreSQL instance with the scheduler database. It does not work with the SQLite backend.

### 5. Quick start

Open four terminals:

```
# Terminal 1: Scheduler
./build/scheduler
```

```
# Terminal 2: Generate tokens (one-time)
./build/admin gen-token --desc "dev-client" --type client
./build/admin gen-token --desc "dev-worker" --type worker
```

```
# Terminal 3: Client
./build/cli register --token <client_token_from_terminal_2>
./build/cli submit --type stress_cpu --param cores=2 --param duration_ms=10000
```

```
# Terminal 4: Worker
./build/worker --token <worker_token_from_terminal_2>
```

After the first run, the client and worker both cache their tokens. Subsequent runs do not need `--token`.
