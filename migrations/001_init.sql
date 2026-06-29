CREATE TABLE IF NOT EXISTS jobs (
  id SERIAL PRIMARY KEY,
  job_type TEXT NOT NULL,
  params TEXT NOT NULL DEFAULT '',
  status TEXT NOT NULL DEFAULT 'not started',
  client_id INTEGER NOT NULL,
  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS clients (
  id SERIAL PRIMARY KEY,
  name TEXT NOT NULL,
  status TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS workers (
  id SERIAL PRIMARY KEY,
  cpu_cores INTEGER NOT NULL,
  mem_size DOUBLE PRECISION NOT NULL,
  disk_size DOUBLE PRECISION NOT NULL,
  cpu_freq DOUBLE PRECISION NOT NULL,
  os TEXT NOT NULL,
  name TEXT NOT NULL,
  status TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS job_worker (
  job_id INTEGER NOT NULL REFERENCES jobs(id),
  worker_id INTEGER NOT NULL REFERENCES workers(id),
  PRIMARY KEY (job_id, worker_id)
);

CREATE TABLE IF NOT EXISTS job_results (
  job_id INTEGER PRIMARY KEY REFERENCES jobs(id),
  success INTEGER NOT NULL,
  message TEXT,
  artifact_url TEXT,
  metrics_json TEXT,
  completed_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS auth_tokens (
  id SERIAL PRIMARY KEY,
  token TEXT NOT NULL UNIQUE,
  description TEXT,
  token_type TEXT NOT NULL DEFAULT 'client' CHECK (token_type IN ('client', 'worker')),
  active INTEGER NOT NULL DEFAULT 1,
  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS client_token_usage (
  id SERIAL PRIMARY KEY,
  client_id INTEGER NOT NULL,
  token_id INTEGER NOT NULL,
  last_used_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  UNIQUE(client_id, token_id)
);

CREATE TABLE IF NOT EXISTS worker_metrics (
  worker_id INTEGER PRIMARY KEY REFERENCES workers(id),
  cpu_percent DOUBLE PRECISION,
  memory_percent DOUBLE PRECISION,
  memory_used_mb DOUBLE PRECISION,
  memory_total_mb DOUBLE PRECISION,
  disk_used_mb DOUBLE PRECISION,
  disk_total_mb DOUBLE PRECISION,
  disk_percent DOUBLE PRECISION,
  rx_bytes_per_sec DOUBLE PRECISION,
  tx_bytes_per_sec DOUBLE PRECISION,
  load_avg_1m DOUBLE PRECISION,
  active_jobs INTEGER,
  reported_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS job_runtime_snapshots (
  job_id INTEGER PRIMARY KEY REFERENCES jobs(id),
  started_at TEXT NOT NULL,
  start_cpu_percent DOUBLE PRECISION DEFAULT 0,
  start_memory_percent DOUBLE PRECISION DEFAULT 0,
  peak_cpu_percent DOUBLE PRECISION DEFAULT 0,
  peak_memory_percent DOUBLE PRECISION DEFAULT 0
);

CREATE TABLE IF NOT EXISTS job_runtime_analytics (
  id SERIAL PRIMARY KEY,
  job_id INTEGER NOT NULL REFERENCES jobs(id),
  job_type TEXT NOT NULL,
  duration_ms INTEGER NOT NULL,
  cpu_spike_percent DOUBLE PRECISION DEFAULT 0,
  memory_spike_percent DOUBLE PRECISION DEFAULT 0,
  peak_cpu_percent DOUBLE PRECISION DEFAULT 0,
  peak_memory_percent DOUBLE PRECISION DEFAULT 0,
  recorded_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS job_ebpf_metrics (
  id SERIAL PRIMARY KEY,
  job_id INTEGER NOT NULL REFERENCES jobs(id),
  worker_id INTEGER NOT NULL REFERENCES workers(id),
  recorded_at DOUBLE PRECISION NOT NULL,
  syscall_read_count BIGINT DEFAULT 0,
  syscall_write_count BIGINT DEFAULT 0,
  syscall_openat_count BIGINT DEFAULT 0,
  io_read_bytes BIGINT DEFAULT 0,
  io_write_bytes BIGINT DEFAULT 0,
  net_tx_bytes BIGINT DEFAULT 0,
  net_rx_bytes BIGINT DEFAULT 0,
  net_conn_count INTEGER DEFAULT 0,
  cpu_usage_us BIGINT DEFAULT 0,
  mem_current_bytes BIGINT DEFAULT 0
);

CREATE INDEX IF NOT EXISTS idx_job_ebpf_metrics_job_ts ON job_ebpf_metrics(job_id, recorded_at);
