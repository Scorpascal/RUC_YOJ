/* Pure-page transport: native cross-site forms, no inferred judge results. */
(() => {
  "use strict";
  const B = window.YOJBatch, S = window.YOJBatchSession, $ = id => document.getElementById(id);
  const transport = window.YOJBatchTransport.createTransport({document});
  const start = $("start"), stop = $("stop"), status = $("status");
  const newBatch = $("new-batch"), inpage = $("inpage"), bookmark = $("inpage-bookmark");
  let items = [], runner = null, ledger = null, entry = null, busy = false, reservedTarget = null;
  const cells = new Map();
  function message(text, error = false) { status.textContent = text; status.className = error ? "error" : ""; }
  function make(tag, text) { const node = document.createElement(tag); if (text !== undefined) node.textContent = text; return node; }
  function openLedger(id, resume = false) {
    const next = S.createLedger(sessionStorage, id, items.map(e => e.problemNo), {resume});
    const url = new URL(location.href);
    url.searchParams.set("batch", id);
    history.replaceState(null, "", url.href);
    ledger = next;
    entry = window.YOJBatchEntry.create(localStorage, navigator.locks, id, items.map(e => e.problemNo));
    const overlaps = ledger.overlaps();
    $("overlap").textContent = overlaps.length
      ? `其他批次曾尝试题目 ${overlaps.join("、")}。本次是独立新批次，可在核对YOJ记录后确认再次提交；不会自动重发。` : "";
  }
  function arm(initial = false) {
    const attempted = new Set([...ledger.attempted(), ...entry.attempted()]), remaining = items.filter(e => !attempted.has(e.problemNo));
    if (initial) items.forEach(e => {
      cells.get(e.problemNo).textContent = attempted.has(e.problemNo) ? "本批次已尝试 · 结果待查" : "已校验 · 等待确认";
    });
    runner = remaining.length ? B.createRunner(remaining, {...transport, openTarget: () => { const target = reservedTarget; reservedTarget = null; return target || transport.openTarget(); }, remember: n => { entry.remember(n); ledger.remember(n); }, onChange: changed}) : null;
    start.disabled = !remaining.length || (entry.mode() || ledger.mode()) === "inpage";
    if (inpage) inpage.disabled = !!attempted.size || !remaining.length || (entry.mode() || ledger.mode()) === "cross";
    start.textContent = attempted.size ? `仅继续未发起的 ${remaining.length} 题` : "跨站直发（兼容入口）";
    newBatch.disabled = false;
    if (!remaining.length) message("本批次已全部尝试，本页不会自动重发；不代表已受理或Accepted。请到YOJ核对结果，需要再次提交时可明确新建批次。");
    else if (initial && attempted.size) message(`本批次已尝试 ${attempted.size} 题；确认后仅继续未发起的 ${remaining.length} 题，不重复已尝试部分。`);
    else if (initial) message(`全部 ${items.length} 题源码哈希校验通过；尚未发送评测。推荐确认此批并打开YOJ，点击保存的书签执行整批；至少间隔3秒，逐题核对受理。跨站兼容入口仍需弹窗检测。`);
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
    start.disabled = true; newBatch.disabled = true; if (inpage) inpage.disabled = true; stop.disabled = !["PREPARING", "RUNNING"].includes(snapshot.phase);
    const text = {
      PREPARING: "正在检测首次及3秒后的弹窗能力；尚未发送任何评测。可停止检测。",
      RUNNING: `本轮已请求发起 ${count} / ${snapshot.rows.length} 题；每题独立响应页，至少间隔3秒。可停止后续提交；浏览器安全提示仍需你处理。`,
      DONE: `队列结束：本轮已请求发起 ${count} / ${snapshot.rows.length} 题，不代表已受理或Accepted。请到YOJ查看评测结果。`,
      STOPPED: `队列已停止：已请求发起 ${count} 题，其余不再发送；状态不明的题目请先核对YOJ记录，禁止自动重发。`,
      BLOCKED: "本轮已阻止发送；请按下方原因处理后重新检测。不会自动重试或重发已尝试题目。",
    };
    message((text[snapshot.phase] || "尚未开始。") + (snapshot.detail ? ` ${snapshot.detail}` : ""), ["STOPPED", "BLOCKED"].includes(snapshot.phase));
    if (snapshot.phase === "BLOCKED" && snapshot.detail.includes("弹窗")) $("popup-help").open = true;
  }
  if (bookmark) {
    const loader = function () {
      if (location.hostname !== "yoj.ruc.edu.cn") { alert("请先打开本批次的 YOJ 页面，再点此书签。"); return; }
      if (window.__yojInPageBatch) { window.__yojInPageBatch.show(); return; }
      if (document.getElementById("yoj-inpage-loader")) return;
      if (!new URLSearchParams(location.search).get("yoj_batch") && !location.hash.startsWith("#yoj-batch=")) { alert("请从题库打开带本批次清单的YOJ页面。"); return; }
      const nonce = Array.from(crypto.getRandomValues(new Uint8Array(16)), b => b.toString(16).padStart(2,"0")).join("");
      const gate = window.open("https://scorpascal.github.io/RUC_YOJ/yoj-batch-gate.html#" + nonce, "_blank");
      if (!gate) { alert("核验窗口被阻止，尚未发送；请允许本次核验窗口。"); return; }
      window.__yojInPageGate = {window: gate, nonce};
      const script = document.createElement("script"); script.id = "yoj-inpage-loader"; script.charset = "UTF-8";
      script.src = "https://scorpascal.github.io/RUC_YOJ/assets/yoj-inpage-batch.js";
      script.onerror = () => { script.remove(); gate.close(); alert("执行器未加载，尚未发送；请检查网络后重试加载。"); };
      document.head.append(script);
    };
    bookmark.href = "javascript:" + encodeURIComponent("(" + loader.toString() + ")();void 0");
  }
  inpage?.addEventListener("click", async () => {
    if (busy || !ledger || ledger.attempted().size || entry.attempted().size) return;
    busy = true; let target;
    try {
      // Open in the original gesture, then claim the batch before navigation.
      target = window.open("about:blank", "_blank");
      if (!target) throw Error("YOJ窗口被阻止，尚未发送；请允许本次窗口。");
      target.opener = null;
      await entry.run("inpage", () => {
        ledger.claimMode("inpage");
        const payload = window.YOJInPageCore.makeHandoff(new URL(location.href).searchParams.get("batch"), items);
        window.YOJInPageCore.saveApproved(localStorage, payload);
        target.location.href = "http://yoj.ruc.edu.cn/index.php/index/submissions/index.html?score_min=0&yoj_batch=" + payload.id;
      });
      arm();
      message("已准备此批：请在新开的YOJ页面点击保存的‘YOJ 整批提交’书签。本站尚未发送评测；同一批次已尝试题目不会自动重发。");
    } catch (error) { try { target?.close(); } catch (_) {} message(error.message || "批次准备失败，尚未发送。", true); }
    finally { busy = false; }
  });
  start.addEventListener("click", () => {
    if (busy || !runner || runner.snapshot().phase !== "READY") return;
    busy = true;
    // Probe in the gesture; the per-batch lock stays held until the queue ends.
    reservedTarget = transport.openTarget();
    entry.run("cross", async () => {
      ledger.claimMode("cross"); arm(); if (inpage) inpage.disabled = true;
      if (runner) await runner.start();
    }).catch(error => { runner?.stop(); message(error.message || "队列异常，已停止，请核对YOJ记录。", true); })
      .finally(() => {
        busy = false; reservedTarget = null; stop.disabled = true;
        try { arm(); }
        catch (error) { start.disabled = true; message(error.message, true); }
      });
  });
  newBatch.addEventListener("click", () => {
    if (busy || !items.length) return;
    try { openLedger(crypto.randomUUID()); arm(true); }
    catch (error) { start.disabled = true; message(error.message || "无法新建批次；尚未发送评测。", true); }
  });
  stop.addEventListener("click", () => runner?.stop());
  window.addEventListener("pagehide", () => runner?.stop());
  window.addEventListener("beforeunload", event => {
    if (busy) { event.preventDefault(); event.returnValue = ""; }
  });
  async function loadJSON(path) {
    const response = await fetch(path, {cache: "no-store", credentials: "omit"});
    if (!response.ok) throw new Error("公开清单下载失败，尚未发送评测。");
    return response.json();
  }
  async function init() {
    const numbers = B.parseNumbers(new URLSearchParams(location.search).get("pnos"));
    const [manifest, catalog] = await Promise.all([loadJSON("data/quick-submit.json"), loadJSON("data/catalog.json")]);
    items = await B.prepareBatch(numbers, manifest, catalog, fetch, crypto);
    render();
    newBatch.disabled = false;
    const suppliedId = new URLSearchParams(location.search).get("batch");
    openLedger(suppliedId || crypto.randomUUID(), !!suppliedId);
    arm(true);
  }
  init().catch(error => { start.disabled = true; message(error.message || "预检失败，尚未发送评测。", true); });
})();
