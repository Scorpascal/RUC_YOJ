/* Pure-page transport: native cross-site forms, no inferred judge results. */
(() => {
  "use strict";
  const B = window.YOJBatch, $ = id => document.getElementById(id);
  const transport = window.YOJBatchTransport.createTransport({document});
  const start = $("start"), stop = $("stop"), status = $("status");
  const ATTEMPTS_KEY = "yoj-batch-attempts-v1";
  let items = [], runner = null, attempted = new Set();
  const cells = new Map();
  function message(text, error = false) { status.textContent = text; status.className = error ? "error" : ""; }
  function make(tag, text) { const node = document.createElement(tag); if (text !== undefined) node.textContent = text; return node; }
  function readAttempts() {
    const saved = JSON.parse(sessionStorage.getItem(ATTEMPTS_KEY) || "[]");
    if (!Array.isArray(saved) || saved.some(n => !Number.isSafeInteger(n) || n <= 0)) throw new Error("本标签页发送记录不可解析；请先核对YOJ记录，未发出新评测。");
    return new Set(saved);
  }
  function remember(n) {
    // Reload the session ledger immediately before each write; never overwrite
    // another batch's attempted problem with a stale in-memory set.
    attempted = readAttempts();
    if (attempted.has(n)) throw new Error("本标签页已尝试过该题，禁止自动重复发送。");
    attempted.add(n);
    sessionStorage.setItem(ATTEMPTS_KEY, JSON.stringify([...attempted]));
  }
  function render() {
    $("count").textContent = `${items.length} / 10`;
    for (const item of items) {
      const tr = make("tr"), name = make("td"), link = make("a", `#${item.problemNo} ${item.title}`);
      link.href = item.problemUrl; link.target = "_blank"; link.rel = "noopener";
      name.append(link);
      const lang = make("td", `${item.language} / ${item.originalSubmissionKind === "PARTIAL_CODE" ? "已核验组装完整代码" : "完整代码"}`);
      const state = make("td"), label = make("span", "已校验 · 等待确认"); label.className = "row-state";
      state.append(label); cells.set(item.problemNo, label); tr.append(name, lang, state); $("queue").append(tr);
      const details = make("details"), summary = make("summary", `查看 #${item.problemNo} 待提交代码（${item.code.length} 字符）`);
      const hash = make("p", `SHA-256: ${item.codeSha256}`); hash.className = "muted";
      details.append(summary, hash, make("pre", item.code)); $("previews").append(details);
    }
  }
  function changed(snapshot) {
    const labels = {WAITING: "尚未发起", REQUESTED: "已请求发起 · 结果待查",
      UNCERTAIN: snapshot.phase === "RUNNING" ? "正在发起 · 尚未确认受理" : "状态不明 · 勿重发"};
    snapshot.rows.forEach(row => { cells.get(row.problemNo).textContent = labels[row.state]; });
    const count = snapshot.rows.filter(row => row.state === "REQUESTED").length;
    start.disabled = true; stop.disabled = snapshot.phase !== "RUNNING";
    const text = {
      RUNNING: `已请求发起 ${count} / ${items.length} 题；每题独立响应页，至少间隔3秒。可停止后续提交；浏览器安全提示仍需你处理。`,
      DONE: `队列结束：已请求发起 ${count} / ${items.length} 题，不代表已受理或Accepted。请到YOJ查看评测结果。`,
      STOPPED: `队列已停止：已请求发起 ${count} 题，其余不再发送；状态不明的题目请先核对YOJ记录，禁止自动重发。`,
      BLOCKED: "未继续发送：本标签页记录不可用/该题已尝试，或本地提交异常。请先核对YOJ记录；未发起部分可重新选题，勿重复发送。",
    };
    message((text[snapshot.phase] || "尚未开始。") + (snapshot.detail ? ` ${snapshot.detail}` : ""), ["STOPPED", "BLOCKED"].includes(snapshot.phase));
  }
  start.addEventListener("click", () => {
    if (!runner || runner.snapshot().phase !== "READY") return;
    // The explicitly labelled start button confirms the visible, fully
    // preflighted batch. Do not ask again per batch or per problem.
    runner.start().catch(() => { runner.stop(); message("队列发生异常，已停止后续发送。请核对YOJ记录，不要自动重发。", true); });
  });
  stop.addEventListener("click", () => runner?.stop());
  window.addEventListener("pagehide", () => runner?.stop());
  window.addEventListener("beforeunload", event => {
    if (runner?.snapshot().phase === "RUNNING") { event.preventDefault(); event.returnValue = ""; }
  });
  async function loadJSON(path) {
    const response = await fetch(path, {cache: "no-store", credentials: "omit"});
    if (!response.ok) throw new Error("公开清单下载失败，尚未发送评测。");
    return response.json();
  }
  async function init() {
    const numbers = B.parseNumbers(new URLSearchParams(location.search).get("pnos"));
    attempted = readAttempts();
    const [manifest, catalog] = await Promise.all([loadJSON("data/quick-submit.json"), loadJSON("data/catalog.json")]);
    items = await B.prepareBatch(numbers, manifest, catalog, fetch, crypto);
    render();
    const duplicates = items.filter(e => attempted.has(e.problemNo));
    if (duplicates.length) throw new Error(`本标签页已尝试题目 ${duplicates.map(e => e.problemNo).join("、")}；请先核对原站记录，本页不会自动重发。未发送的题目可返回主页重新选择。`);
    runner = B.createRunner(items, {...transport, remember, onChange: changed});
    start.disabled = false;
    message(`全部 ${items.length} 题源码哈希校验通过；尚未发送评测。确认后将依次发起，完整队列至少需要 ${(items.length - 1) * B.INTERVAL / 1000} 秒，另加浏览器确认和响应时间。`);
  }
  init().catch(error => { start.disabled = true; message(error.message || "预检失败，尚未发送评测。", true); });
})();
