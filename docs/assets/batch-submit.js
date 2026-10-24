/* Shared validation and one-shot queue. No login, judge reads or background service. */
(function (root, factory) {
  const api = factory();
  if (typeof module === "object" && module.exports) module.exports = api;
  else root.YOJBatch = api;
})(typeof globalThis !== "undefined" ? globalThis : this, function () {
  "use strict";
  const MAX = 10, INTERVAL = 3000;
  const RAW = "https://raw.githubusercontent.com/Scorpascal/RUC_YOJ/main/";
  const STANDARD = "/index.php/index/index/prob_submit.html";
  const PARTIAL = "/index.php/index/index/prob_submit_tk.html";

  function parseNumbers(text) {
    const parts = String(text ?? "").split(",").map(s => s.trim());
    if (parts.length < 1 || parts.length > MAX || parts.some(s => !/^[1-9]\d*$/.test(s)))
      throw new Error("请选择 1–10 道不同的有效题目。");
    const numbers = parts.map(Number);
    if (numbers.some(n => !Number.isSafeInteger(n)) || new Set(numbers).size !== numbers.length)
      throw new Error("题号不能重复或超出有效范围。");
    return numbers;
  }
  function judgeURL(value) {
    const url = new URL(value);
    if (url.hostname !== "yoj.ruc.edu.cn" || !["http:", "https:"].includes(url.protocol) ||
        url.username || url.password || url.port || url.search || url.hash)
      throw new Error("原站链接不受支持。");
    return url;
  }
  function checkedEntry(e, c) {
    if (!e || !c || c.verified !== true || c.publicationStatus !== "PUBLIC_READY" ||
        c.onlineVerificationStatus !== "ONLINE_ACCEPTED" || e.onlineStatus !== "Accepted" ||
        (e.source && e.source !== "PUBLIC_READY"))
      throw new Error("存在未通过 PUBLIC_READY/在线复验的题目，未发送任何评测。");
    if (c.codeSource !== "PUBLIC_READY" || c.quickSubmitMode !== "verified" ||
        c.codeUrl !== e.codeUrl || c.language !== e.language || c.problemUrl !== e.problemUrl ||
        !/^[1-9]\d*$/.test(String(e.onlineSubmissionNo || "")) ||
        String(c.submissionNo) !== String(e.onlineSubmissionNo) ||
        String(c.onlineVerificationSubmissionNo) !== String(e.onlineSubmissionNo))
      throw new Error("题目目录和快捷清单的代码身份或复验证据不一致，未发送评测。");
    const problem = judgeURL(e.problemUrl), endpoint = judgeURL(e.submitEndpoint);
    const match = problem.pathname.match(/^\/index\.php\/index\/problem\/detail\/pno\/(\d+)(?:\/eno\/([1-9]\d*))?\.html$/);
    const kind = e.submissionKind || "STANDARD_CODE";
    if (!match || Number(match[1]) !== e.problemNo || !["STANDARD_CODE", "PARTIAL_CODE"].includes(kind) ||
        endpoint.pathname !== (kind === "PARTIAL_CODE" ? PARTIAL : STANDARD) ||
        typeof e.language !== "string" || !/^[a-zA-Z0-9+_.-]{1,40}$/.test(e.language))
      throw new Error("题号、语言或提交形态不受支持。");
    checkedSource(e);
    if (kind === "PARTIAL_CODE") {
      const native = e.nativeSubmission, mirror = c.nativeSubmission;
      const keys = ["kind", "codePath", "codeUrl", "codeSha256", "submitEndpoint"];
      const exactFields = value => value && typeof value === "object" && !Array.isArray(value) &&
        Object.keys(value).length === keys.length && keys.every(key => Object.hasOwn(value, key));
      if (!exactFields(native) || !exactFields(mirror) || native.kind !== "VERIFIED_ASSEMBLED_CODE" ||
          keys.some(key => native[key] !== mirror[key]) ||
          judgeURL(native.submitEndpoint).pathname !== STANDARD)
        throw new Error("单填空题缺少一致的已核验完整程序证据，未发送评测。");
      checkedSource(native);
    } else if (e.nativeSubmission || c.nativeSubmission) throw new Error("普通题含未知替代提交配置。");
    return Object.freeze({...e, submissionKind: kind});
  }
  function checkedSource(e) {
    const source = new URL(e.codeUrl);
    if (!e.codePath || !/^(?:代码库|题解)\//.test(e.codePath) ||
        e.codePath.split("/").some(p => p === "." || p === ".." || !p) ||
        !source.href.startsWith(RAW) || source.search || source.hash ||
        decodeURIComponent(source.href.slice(RAW.length)) !== e.codePath ||
        !/^[a-f0-9]{64}$/.test(e.codeSha256))
      throw new Error("公开源码地址或 SHA-256 无效。");
  }
  async function prepareBatch(numbers, manifest, catalog, fetcher, cryptoAPI) {
    const ids = parseNumbers(numbers.join(","));
    if (manifest?.schemaVersion !== 2 || !Array.isArray(manifest.entries) || !Array.isArray(catalog?.entries))
      throw new Error("公开清单不可用，请稍后刷新。");
    // Check every record before any source load, never use archivedEntries.
    const entries = ids.map(n => {
      const es = manifest.entries.filter(e => e.problemNo === n);
      const cs = catalog.entries.filter(e => e.problemNo === n);
      if (es.length !== 1 || cs.length !== 1) throw new Error(`题目 ${n} 不存在或清单不唯一。`);
      return checkedEntry(es[0], cs[0]);
    });
    async function verifiedCode(entry) {
      const response = await fetcher(entry.codeUrl, {cache: "no-store", credentials: "omit", redirect: "error"});
      if (!response.ok) throw new Error(`题目 ${entry.problemNo} 源码下载失败，未发送任何评测。`);
      const bytes = new Uint8Array(await response.arrayBuffer());
      if (!bytes.length || bytes.length > 1048576) throw new Error("源码为空或超出 1 MiB 限制。");
      const hash = Array.from(new Uint8Array(await cryptoAPI.subtle.digest("SHA-256", bytes)), b => b.toString(16).padStart(2, "0")).join("");
      if (hash !== entry.codeSha256) throw new Error(`题目 ${entry.problemNo} 源码哈希不匹配，未发送任何评测。`);
      const code = new TextDecoder("utf-8", {fatal: true, ignoreBOM: true}).decode(bytes);
      if (!code.trim()) throw new Error("源码内容为空，未发送评测。");
      return code;
    }
    const ready = [];
    for (const entry of entries) {
      const code = await verifiedCode(entry);
      if (entry.submissionKind === "PARTIAL_CODE") {
        const native = entry.nativeSubmission;
        const fullCode = await verifiedCode({...native, problemNo: entry.problemNo});
        ready.push(Object.freeze({...entry, ...native, code: fullCode,
          submissionKind: "STANDARD_CODE", originalSubmissionKind: "PARTIAL_CODE", fragmentCode: code}));
      } else ready.push(Object.freeze({...entry, code}));
    }
    return Object.freeze(ready);
  }
  function formFields(item) {
    return item.submissionKind === "PARTIAL_CODE"
      ? {pno: String(item.problemNo), compiler: item.language, codes: JSON.stringify([item.code])}
      : {pid: String(item.problemNo), language: item.language, code: item.code};
  }
  function pause(ms, signal) {
    return new Promise(resolve => {
      function done() { clearTimeout(timer); signal.removeEventListener("abort", done); resolve(); }
      const timer = setTimeout(done, ms);
      signal.addEventListener("abort", done, {once: true});
      if (signal.aborted) done();
    });
  }
  function createRunner(items, io) {
    parseNumbers(items.map(item => item.problemNo).join(","));
    const rows = items.map(item => ({problemNo: item.problemNo, state: "WAITING"}));
    let phase = "READY", used = false, detail = "";
    const controller = new AbortController();
    const snapshot = () => ({phase, detail, rows: rows.map(row => ({...row}))});
    const notify = () => io.onChange?.(snapshot());
    async function start() {
      if (used) return;
      used = true;
      let target;
      phase = "PREPARING"; notify();
      try {
        target = io.openTarget();
        if (!target || target.closed) throw new Error("提交环境不可用。");
        if (io.prepare) await io.prepare(target, controller.signal);
      } catch (error) {
        phase = controller.signal.aborted ? "STOPPED" : "BLOCKED";
        detail = String(error?.message || "提交环境检测失败").slice(0, 240); notify(); return;
      }
      if (controller.signal.aborted) { phase = "STOPPED"; notify(); return; }
      phase = "RUNNING"; notify();
      for (let i = 0; i < items.length; i++) {
        if (controller.signal.aborted) break;
        if (target.closed) { phase = "STOPPED"; break; }
        try { io.remember(items[i].problemNo); }
        catch (_) { phase = "BLOCKED"; break; }
        rows[i].state = "UNCERTAIN";
        notify();
        try { await io.send(items[i], target, controller.signal); }
        catch (error) { phase = "STOPPED"; detail = String(error?.message || "响应未确认").slice(0, 240); notify(); return; }
        rows[i].state = "REQUESTED"; notify();
        if (i < items.length - 1) await (io.pause || pause)(INTERVAL, controller.signal);
      }
      if (phase === "RUNNING") phase = controller.signal.aborted ? "STOPPED" : "DONE";
      notify();
    }
    function stop() {
      if (!["PREPARING", "RUNNING"].includes(phase)) return;
      phase = "STOPPED"; controller.abort(); notify();
    }
    return Object.freeze({start, stop, snapshot});
  }
  return Object.freeze({MAX, INTERVAL, parseNumbers, prepareBatch, formFields, createRunner});
});
