const express = require('express');
const path = require('path');
const pool = require('./db');

const app = express();
const PORT = process.env.PORT || 3000;

app.set('view engine', 'ejs');
app.set('views', path.join(__dirname, 'views'));
app.use(express.static(path.join(__dirname, 'public')));


app.get('/', async (req, res) => {
  try {
    const [workers, jobs, results, active] = await Promise.all([
      pool.query('SELECT COUNT(*)::int AS c FROM workers'),
      pool.query('SELECT COUNT(*)::int AS c FROM jobs'),
      pool.query("SELECT COUNT(*)::int AS c FROM job_results WHERE success = 1"),
      pool.query("SELECT COUNT(*)::int AS c FROM jobs WHERE status = 'started'"),
    ]);
    const recent = await pool.query(`
      SELECT j.id, j.job_type, j.status, j.created_at,
             w.name AS worker_name, jr.success
      FROM jobs j
      LEFT JOIN job_worker jw ON jw.job_id = j.id
      LEFT JOIN workers w ON w.id = jw.worker_id
      LEFT JOIN job_results jr ON jr.job_id = j.id
      ORDER BY j.created_at DESC LIMIT 20
    `);
    res.render('index', {
      total_workers: workers.rows[0].c,
      total_jobs: jobs.rows[0].c,
      succeeded: results.rows[0].c,
      active_jobs: active.rows[0].c,
      recent: recent.rows,
    });
  } catch (err) {
    console.error(err);
    res.status(500).send('Database error');
  }
});

app.get('/workers', async (req, res) => {
  try {
    const r = await pool.query(`
      SELECT w.*, wm.cpu_percent, wm.memory_percent, wm.memory_used_mb,
             wm.memory_total_mb, wm.disk_percent, wm.load_avg_1m, wm.reported_at,
             (SELECT COUNT(*) FROM job_worker jw2 WHERE jw2.worker_id = w.id) AS job_count
      FROM workers w
      LEFT JOIN worker_metrics wm ON wm.worker_id = w.id
      ORDER BY w.id
    `);
    res.render('workers', { workers: r.rows });
  } catch (err) {
    console.error(err);
    res.status(500).send('Database error');
  }
});

app.get('/workers/:id', async (req, res) => {
  try {
    const id = parseInt(req.params.id);
    const [winfo, active, history] = await Promise.all([
      pool.query(`
        SELECT w.*, wm.cpu_percent, wm.memory_percent, wm.memory_used_mb,
               wm.memory_total_mb, wm.disk_percent, wm.load_avg_1m, wm.reported_at
        FROM workers w
        LEFT JOIN worker_metrics wm ON wm.worker_id = w.id
        WHERE w.id = $1
      `, [id]),
      pool.query(`
        SELECT j.*, jr.success, jr.message
        FROM jobs j
        JOIN job_worker jw ON jw.job_id = j.id
        LEFT JOIN job_results jr ON jr.job_id = j.id
        WHERE jw.worker_id = $1 AND j.status IN ('started','ongoing','scheduled')
        ORDER BY j.created_at DESC
      `, [id]),
      pool.query(`
        SELECT j.*, jr.success, jr.message, jr.completed_at,
               jra.duration_ms, jra.cpu_spike_percent, jra.memory_spike_percent
        FROM jobs j
        JOIN job_worker jw ON jw.job_id = j.id
        LEFT JOIN job_results jr ON jr.job_id = j.id
        LEFT JOIN job_runtime_analytics jra ON jra.job_id = j.id
        WHERE jw.worker_id = $1 AND j.status IN ('completed','failed')
        ORDER BY j.updated_at DESC LIMIT 100
      `, [id]),
    ]);
    if (winfo.rows.length === 0) return res.status(404).send('Worker not found');
    res.render('worker', {
      worker: winfo.rows[0],
      active_jobs: active.rows,
      history: history.rows,
    });
  } catch (err) {
    console.error(err);
    res.status(500).send('Database error');
  }
});

app.get('/jobs', async (req, res) => {
  try {
    const status = req.query.status;
    let q = `
      SELECT j.*, w.name AS worker_name, jr.success, jr.message
      FROM jobs j
      LEFT JOIN job_worker jw ON jw.job_id = j.id
      LEFT JOIN workers w ON w.id = jw.worker_id
      LEFT JOIN job_results jr ON jr.job_id = j.id
    `;
    const params = [];
    if (status) {
      q += ' WHERE j.status = $1';
      params.push(status);
    }
    q += ' ORDER BY j.created_at DESC LIMIT 200';
    const r = await pool.query(q, params);
    res.render('jobs', { jobs: r.rows, current_status: status || '' });
  } catch (err) {
    console.error(err);
    res.status(500).send('Database error');
  }
});

app.get('/jobs/:id', async (req, res) => {
  try {
    const id = parseInt(req.params.id);
    const [jinfo, ebpf] = await Promise.all([
      pool.query(`
        SELECT j.*, w.name AS worker_name, jr.success, jr.message, jr.completed_at,
               jra.duration_ms, jra.cpu_spike_percent, jra.memory_spike_percent
        FROM jobs j
        LEFT JOIN job_worker jw ON jw.job_id = j.id
        LEFT JOIN workers w ON w.id = jw.worker_id
        LEFT JOIN job_results jr ON jr.job_id = j.id
        LEFT JOIN job_runtime_analytics jra ON jra.job_id = j.id
        WHERE j.id = $1
      `, [id]),
      pool.query(`
        SELECT * FROM job_ebpf_metrics
        WHERE job_id = $1 ORDER BY recorded_at
      `, [id]),
    ]);
    if (jinfo.rows.length === 0) return res.status(404).send('Job not found');
    res.render('job', { job: jinfo.rows[0], ebpf: ebpf.rows });
  } catch (err) {
    console.error(err);
    res.status(500).send('Database error');
  }
});


app.get('/api/workers/:id/metrics', async (req, res) => {
  try {
    const r = await pool.query(`
      SELECT reported_at, cpu_percent, memory_percent, memory_used_mb,
             load_avg_1m, rx_bytes_per_sec, tx_bytes_per_sec
      FROM worker_metrics WHERE worker_id = $1
      ORDER BY reported_at DESC LIMIT 60
    `, [parseInt(req.params.id)]);
    res.json(r.rows.reverse());
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.get('/api/workers/:id/jobs', async (req, res) => {
  try {
    const r = await pool.query(`
      SELECT j.id, j.job_type, j.status, j.created_at, j.updated_at,
             jr.success, jr.message, jra.duration_ms
      FROM jobs j
      JOIN job_worker jw ON jw.job_id = j.id
      LEFT JOIN job_results jr ON jr.job_id = j.id
      LEFT JOIN job_runtime_analytics jra ON jra.job_id = j.id
      WHERE jw.worker_id = $1
      ORDER BY j.created_at DESC LIMIT 100
    `, [parseInt(req.params.id)]);
    res.json(r.rows);
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.get('/api/jobs/:id/ebpf', async (req, res) => {
  try {
    const r = await pool.query(`
      SELECT recorded_at, syscall_read_count, syscall_write_count,
             syscall_openat_count, io_read_bytes, io_write_bytes,
             net_tx_bytes, net_rx_bytes, cpu_usage_us, mem_current_bytes
      FROM job_ebpf_metrics WHERE job_id = $1
      ORDER BY recorded_at
    `, [parseInt(req.params.id)]);
    res.json(r.rows);
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});


app.listen(PORT, () => {
  console.log(`Dashboard at http://localhost:${PORT}`);
});
