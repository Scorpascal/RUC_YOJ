/* Pure-page transport: native cross-site forms, no inferred judge results. */
(() => {
  "use strict";
  const B = window.YOJBatch, $ = id => document.getElementById(id);
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
      const lang = make("td", `${item.language} / ${item.submissionKind === "PARTIAL_CODE" ? "单填空片段" : "完整代码"}`);
      const state = make("td"), label = make("span", "已校验 · 等待确认"); label.className = "row-state";
      state.append(label); cells.set(item.problemNo, label); tr.append(name, lang, state); $("queue").append(tr);
      const details = make("details"), summary = make("summary", `查看 #${item.problemNo} 待提交代码（${item.code.length} 字符）`);
      const hash = make("p", `SHA-256: ${item.codeSha256}`); hash.className = "muted";
      details.append(summary, hash, make("pre", item.code)); $("previews").append(details);
    }
  }
  function openTarget() {
    const name = "yoj_batch_" + crypto.randomUUID().replaceAll("-", "");
    // Exactly one window opened synchronously inside the confirmed click.
    const win = window.open("about:blank", name);
    if (!win) return null;
    return {name, get closed() { return win.closed; }};
  }
  function send(item, target) {
    const form = make("form"); form.method = "post"; form.action = item.submitEndpoint;
    form.target = target; form.acceptCharset = "UTF-8";
    for (const [name, value] of Object.entries(B.formFields(item))) {
      const input = make("input"); input.type = "hidden"; input.name = name; input.value = value; form.append(input);
    }
    document.body.append(form);
    try { form.submit(); } finally { form.remove(); }
  }
  function changed(snapshot) {
    const labels = {WAITING: "尚未发起", REQUESTED: "已请求发起 · 结果待查", UNCERTAIN: "状态不明 · 勿重发"};
    snapshot.rows.forEach(row => { cells.get(row.problemNo).textContent = labels[row.state]; });
    const count = snapshot.rows.filter(row => row.state === "REQUESTED").length;
    start.disabled = true; stop.disabled = snapshot.phase !== "RUNNING";
    const text = {
      RUNNING: `已请求发起 ${count} / ${items.length} 题；下一题至少间隔3秒。请在YOJ核对受理与评测结果。`,
      DONE: `队列结束：已请求发起 ${count} / ${items.length} 题，不代表已受理或Accepted。请到YOJ查看评测结果。`,
      STOPPED: `队列已停止：已请求发起 ${count} 题，其余不再发送；状态不明的题目请先核对YOJ记录，禁止自动重发。`,
      BLOCKED: "未继续发送：响应窗口被阻止/关闭，或本标签页记录不可用/该题已尝试。请先核对YOJ记录；未发起部分可重新选题，勿重复发送。",
    };
    message(text[snapshot.phase] || "尚未开始。", ["STOPPED", "BLOCKED"].includes(snapshot.phase));
  }
  start.addEventListener("click", () => {
    if (!runner || runner.snapshot().phase !== "READY") return;
    const summary = items.map(e => `#${e.problemNo} · ${e.language} · ${e.submissionKind === "PARTIAL_CODE" ? "单填空" : "完整代码"}`).join("\n");
    if (!window.confirm(`确认批量提交以下 ${items.length} 题？\n${summary}\n\n每题间隔3秒；须已登录YOJ，特殊题须有权限。这里只发起请求，结果到YOJ查看；发起后不可撤销。`)) return;
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
    runner = B.createRunner(items, {openTarget, send, remember, onChange: changed});
    start.disabled = false;
    message(`全部 ${items.length} 题源码哈希校验通过；尚未发送评测。确认后将依次发起，完整队列至少需要 ${(items.length - 1) * B.INTERVAL / 1000} 秒。`);
  }
  init().catch(error => { start.disabled = true; message(error.message || "预检失败，尚未发送评测。", true); });
})();
