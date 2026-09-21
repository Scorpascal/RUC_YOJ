CREATE TABLE IF NOT EXISTS traffic_meta (
  id INTEGER PRIMARY KEY CHECK (id = 1),
  total_visits INTEGER NOT NULL DEFAULT 0 CHECK (total_visits >= 0)
);

INSERT OR IGNORE INTO traffic_meta (id, total_visits) VALUES (1, 0);

CREATE TABLE IF NOT EXISTS traffic_daily (
  date TEXT PRIMARY KEY,
  visits INTEGER NOT NULL DEFAULT 0 CHECK (visits >= 0)
);
