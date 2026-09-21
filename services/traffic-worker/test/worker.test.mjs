import assert from "node:assert/strict";
import test from "node:test";
import worker from "../src/index.js";

class FakeStatement {
  constructor(sql) {
    this.sql = sql;
    this.bindings = [];
  }

  bind(...bindings) {
    this.bindings = bindings;
    return this;
  }
}

class FakeD1 {
  constructor() {
    this.total = 0;
    this.daily = new Map();
  }

  prepare(sql) {
    return new FakeStatement(sql);
  }

  async batch(statements) {
    return statements.map((statement) => {
      if (statement.sql.startsWith("UPDATE traffic_meta")) {
        this.total += 1;
        return { success: true, results: [] };
      }
      if (statement.sql.startsWith("INSERT INTO traffic_daily")) {
        const date = statement.bindings[0];
        this.daily.set(date, (this.daily.get(date) || 0) + 1);
        return { success: true, results: [] };
      }
      if (statement.sql.startsWith("SELECT total_visits")) {
        return { success: true, results: [{ total_visits: this.total }] };
      }
      if (statement.sql.startsWith("SELECT date, visits")) {
        const [start, end] = statement.bindings;
        const results = [...this.daily.entries()]
          .filter(([date]) => date >= start && date <= end)
          .sort(([left], [right]) => left.localeCompare(right))
          .map(([date, visits]) => ({ date, visits }));
        return { success: true, results };
      }
      return { success: true, results: [] };
    });
  }
}

function env() {
  return { DB: new FakeD1(), ALLOWED_ORIGIN: "https://scorpascal.github.io" };
}

test("visit writes one daily and one total count, then returns the same snapshot", async () => {
  const runtime = env();
  const first = await worker.fetch(
    new Request("https://traffic.example/api/visit", {
      method: "POST",
      headers: { Origin: "https://scorpascal.github.io" },
    }),
    runtime,
  );
  const firstPayload = await first.json();

  assert.equal(first.status, 200);
  assert.equal(firstPayload.todayVisits, 1);
  assert.equal(firstPayload.totalVisits, 1);
  assert.equal(firstPayload.series.length, 7);
  assert.equal(firstPayload.series.at(-1).visits, firstPayload.todayVisits);

  const second = await worker.fetch(
    new Request("https://traffic.example/api/visit", {
      method: "POST",
      headers: { Origin: "https://scorpascal.github.io" },
    }),
    runtime,
  );
  const secondPayload = await second.json();
  assert.equal(secondPayload.todayVisits, 2);
  assert.equal(secondPayload.totalVisits, 2);
});

test("visit rejects a missing or foreign Origin without changing the counter", async () => {
  const runtime = env();
  const response = await worker.fetch(
    new Request("https://traffic.example/api/visit", { method: "POST" }),
    runtime,
  );
  assert.equal(response.status, 403);
  assert.equal(runtime.DB.total, 0);
});

test("read-only stats exposes the current seven-day snapshot", async () => {
  const runtime = env();
  await worker.fetch(
    new Request("https://traffic.example/api/visit", {
      method: "POST",
      headers: { Origin: "https://scorpascal.github.io" },
    }),
    runtime,
  );
  const response = await worker.fetch(new Request("https://traffic.example/api/stats"), runtime);
  const payload = await response.json();
  assert.equal(response.status, 200);
  assert.equal(payload.totalVisits, 1);
  assert.equal(payload.series.at(-1).visits, 1);
});
