/* One ledger per explicit batch identity; stores only IDs and problem numbers. */
(function (root, factory) {
  if (typeof module === "object" && module.exports) module.exports = factory(require("./batch-submit.js"));
  else root.YOJBatchSession = factory(root.YOJBatch);
})(typeof globalThis !== "undefined" ? globalThis : this, function (B) {
  "use strict";
  const KEY = "yoj-batch-sessions-v2";
  const validId = id => typeof id === "string" && /^[a-f0-9]{8}(?:-[a-f0-9]{4}){3}-[a-f0-9]{12}$/i.test(id);
  function createLedger(storage, id, numbers, {resume = false} = {}) {
    if (!validId(id)) throw new Error("批次编号无效；请明确新建批次，未发送评测。");
    const selection = B.parseNumbers(numbers.join(","));
    const same = (a, b) => a.length === b.length && a.every((n, i) => n === b[i]);
    function read() {
      let data;
      try { data = JSON.parse(storage.getItem(KEY) || '{"version":2,"batches":[]}'); }
      catch (_) { throw new Error("批次记录不可读取；请先核对YOJ记录，未发送新评测。"); }
      if (data?.version !== 2 || !Array.isArray(data.batches)) throw new Error("批次记录损坏；未发送新评测。");
      const seen = new Set();
      for (const row of data.batches) {
        if (!row || !validId(row.id) || seen.has(row.id) || !Array.isArray(row.selection) || !Array.isArray(row.attempted))
          throw new Error("批次记录损坏；未发送新评测。");
        B.parseNumbers(row.selection.join(","));
        if (row.attempted.some(n => !row.selection.includes(n)) || new Set(row.attempted).size !== row.attempted.length)
          throw new Error("批次尝试记录损坏；未发送新评测。");
        seen.add(row.id);
      }
      return data;
    }
    function own(data) {
      const row = data.batches.find(r => r.id === id);
      if (!row || !same(row.selection, selection)) throw new Error("本批次题目选择不一致；请明确新建批次，未发送评测。");
      return row;
    }
    const initial = read();
    if (!initial.batches.some(row => row.id === id)) {
      if (resume) throw new Error("找不到本批次的发送记录；请先核对YOJ，再明确新建批次。未发送任何评测。");
      initial.batches.push({id, selection, attempted: []});
      storage.setItem(KEY, JSON.stringify(initial));
    }
    own(initial);
    function attempted() { return new Set(own(read()).attempted); }
    function remember(n) {
      const data = read(), row = own(data);
      if (!selection.includes(n) || row.attempted.includes(n)) throw new Error("本批次题目不匹配或已尝试；禁止自动重发。");
      row.attempted.push(n);
      storage.setItem(KEY, JSON.stringify(data));
    }
    function overlaps() {
      const prior = new Set(read().batches.filter(row => row.id !== id).flatMap(row => row.attempted));
      // v1 is advisory only: it cannot distinguish separate user intents.
      try {
        const legacy = JSON.parse(storage.getItem("yoj-batch-attempts-v1") || "[]");
        if (Array.isArray(legacy)) legacy.forEach(n => { if (Number.isSafeInteger(n) && n > 0) prior.add(n); });
      } catch (_) { /* Old-format data never authorizes a send or blocks a new intent. */ }
      return selection.filter(n => prior.has(n));
    }
    return Object.freeze({attempted, remember, overlaps});
  }
  return Object.freeze({createLedger});
});
