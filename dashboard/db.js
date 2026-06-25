const { Pool } = require('pg');

const pool = new Pool({
  host: process.env.PGHOST || 'localhost',
  port: parseInt(process.env.PGPORT || '5432'),
  database: process.env.PGDATABASE || 'djs',
  user: process.env.PGUSER || 'djs',
  password: process.env.PGPASSWORD || 'djs',
});

module.exports = pool;
