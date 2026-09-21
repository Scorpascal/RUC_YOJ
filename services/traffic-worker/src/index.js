const PAGE_ID = "Scorpascal.RUC_YOJ";
const TIMEZONE_NAME = "Asia/Shanghai";
const DEFAULT_ALLOWED_ORIGIN = "https://scorpascal.github.io";
const SCHEMA_VERSION = 1;

function configuredOrigins(env) {
  return String(env?.ALLOWED_ORIGIN || DEFAULT_ALLOWED_ORIGIN)
    .split(",")
    .map((value) => value.trim())
    .filter(Boolean);
}

function requestOriginAllowed(request, env, requireOrigin = false) {
  const origin = request.headers.get("Origin");
  if (!origin) return !requireOrigin;
  return configuredOrigins(env).includes(origin);
}

function corsHeaders(request, env) {
  const origin = request.headers.get("Origin");
  const headers = {
    "Cache-Control": "no-store",
    "Content-Type": "application/json; charset=utf-8",
    Vary: "Origin",
  };
  if (origin && configuredOrigins(env).includes(origin)) {
    headers["Access-Control-Allow-Origin"] = origin;
    headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
    headers["Access-Control-Allow-Headers"] = "Accept, Content-Type";
    headers["Access-Control-Max-Age"] = "86400";
  }
  return headers;
}

function json(data, status, headers) {
  return new Response(JSON.stringify(data), { status, headers });
}

function dateInBeijing(now = new Date()) {
  const parts = new Intl.DateTimeFormat("en-US", {
    timeZone: TIMEZONE_NAME,
    year: "numeric",
    month: "2-digit",
    day: "2-digit",
  }).formatToParts(now);
  const value = (type) => parts.find((part) => part.type === type)?.value;
  return `${value("year")}-${value("month")}-${value("day")}`;
}

function shiftDate(isoDate, offset) {
  const [year, month, day] = isoDate.split("-").map(Number);
  return new Date(Date.UTC(year, month - 1, day + offset)).toISOString().slice(0, 10);
}

function dateLabel(isoDate) {
  const [, month, day] = isoDate.split("-").map(Number);
  return `${month}/${day}`;
}

function rowsOf(result) {
  return Array.isArray(result?.results) ? result.results : [];
}

function snapshotFromRows(today, totalResult, dailyResult, now = new Date()) {
  const totalRow = rowsOf(totalResult)[0] || {};
  const totalVisits = Number(totalRow.total_visits ?? 0);
  if (!Number.isInteger(totalVisits) || totalVisits < 0) {
    throw new Error("traffic_meta returned an invalid total");
  }

  const daily = new Map();
  for (const row of rowsOf(dailyResult)) {
    const visits = Number(row.visits);
    if (typeof row.date !== "string" || !Number.isInteger(visits) || visits < 0) {
      throw new Error("traffic_daily returned an invalid row");
    }
    daily.set(row.date, visits);
  }

  const series = Array.from({ length: 7 }, (_, index) => {
    const date = shiftDate(today, index - 6);
    return { date, label: dateLabel(date), visits: daily.get(date) ?? 0 };
  });

  return {
    schemaVersion: SCHEMA_VERSION,
    source: {
      provider: "ruc-yoj-traffic-worker",
      pageId: PAGE_ID,
      timezone: TIMEZONE_NAME,
      countSemantics: "page_loads",
    },
    updatedAt: now.toISOString(),
    today,
    todayVisits: series[series.length - 1].visits,
    totalVisits,
    series,
  };
}

async function readSnapshot(env, today = dateInBeijing()) {
  const [totalResult, dailyResult] = await env.DB.batch([
    env.DB.prepare("SELECT total_visits FROM traffic_meta WHERE id = 1"),
    env.DB.prepare(
      "SELECT date, visits FROM traffic_daily WHERE date >= ? AND date <= ? ORDER BY date ASC",
    ).bind(shiftDate(today, -6), today),
  ]);
  return snapshotFromRows(today, totalResult, dailyResult);
}

async function incrementAndReadSnapshot(env) {
  const today = dateInBeijing();
  const [ignored, updatedMeta, updatedDaily, totalResult, dailyResult] = await env.DB.batch([
    env.DB.prepare("INSERT OR IGNORE INTO traffic_meta (id, total_visits) VALUES (1, 0)"),
    env.DB.prepare("UPDATE traffic_meta SET total_visits = total_visits + 1 WHERE id = 1"),
    env.DB.prepare(
      "INSERT INTO traffic_daily (date, visits) VALUES (?, 1) ON CONFLICT(date) DO UPDATE SET visits = traffic_daily.visits + 1",
    ).bind(today),
    env.DB.prepare("SELECT total_visits FROM traffic_meta WHERE id = 1"),
    env.DB.prepare(
      "SELECT date, visits FROM traffic_daily WHERE date >= ? AND date <= ? ORDER BY date ASC",
    ).bind(shiftDate(today, -6), today),
  ]);
  void ignored;
  void updatedMeta;
  void updatedDaily;
  return snapshotFromRows(today, totalResult, dailyResult);
}

function healthPayload() {
  return {
    ok: true,
    service: "ruc-yoj-traffic-worker",
    schemaVersion: SCHEMA_VERSION,
    timezone: TIMEZONE_NAME,
  };
}

const worker = {
  async fetch(request, env) {
    const url = new URL(request.url);
    const headers = corsHeaders(request, env);

    if (request.method === "OPTIONS") {
      return new Response(null, {
        status: requestOriginAllowed(request, env) ? 204 : 403,
        headers,
      });
    }

    if (url.pathname === "/health" && request.method === "GET") {
      return json(healthPayload(), 200, headers);
    }

    if (url.pathname === "/api/stats" && request.method === "GET") {
      try {
        return json(await readSnapshot(env), 200, headers);
      } catch (error) {
        return json({ error: "stats_unavailable" }, 503, headers);
      }
    }

    if (url.pathname === "/api/visit" && request.method === "POST") {
      // A browser cross-origin POST always carries Origin. Requiring it here
      // blocks ordinary curl spam while keeping the public Pages flow keyless.
      if (!requestOriginAllowed(request, env, true)) {
        return json({ error: "origin_not_allowed" }, 403, headers);
      }
      try {
        return json(await incrementAndReadSnapshot(env), 200, headers);
      } catch (error) {
        return json({ error: "visit_unavailable" }, 503, headers);
      }
    }

    return json({ error: "not_found" }, 404, headers);
  },
};

export const __testing = { dateInBeijing, shiftDate, snapshotFromRows };
export default worker;
