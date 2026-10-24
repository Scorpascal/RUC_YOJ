/* Direct native POSTs, independent response pages. Never reads results or retries. */
(function (root, factory) {
  if (typeof module === "object" && module.exports) module.exports = factory(require("./batch-submit.js"));
  else root.YOJBatchTransport = factory(root.YOJBatch);
})(typeof globalThis !== "undefined" ? globalThis : this, function (B) {
  "use strict";
  function wait(ms, signal) {
    return new Promise(resolve => {
      function done() { clearTimeout(timer); signal.removeEventListener("abort", done); resolve(); }
      const timer = setTimeout(done, ms);
      signal.addEventListener("abort", done, {once: true});
      if (signal.aborted) done();
    });
  }
  function createTransport({document: owner = globalThis.document,
    openWindow = (...args) => globalThis.open(...args), pause = wait} = {}) {
    const probes = new WeakMap(), ready = new WeakSet();
    function probe() {
      // Empty, same-origin blank probe only. Never navigate a probe to YOJ,
      // reuse its target, or infer a judge receipt from this capability check.
      const popup = openWindow("about:blank", "_blank");
      if (!popup || popup.closed) return false;
      popup.close(); return true;
    }
    // This is a batch descriptor, NOT a claim that any popup was opened.
    // Native target=_blank avoids preopened/named cross-site window reuse.
    function openTarget() {
      const target = Object.freeze({name: "_blank", closed: false});
      probes.set(target, probe()); return target;
    }
    async function prepare(target, signal) {
      if (!probes.get(target)) throw new Error("弹窗检测失败：请仅为本站允许弹出式窗口，再点击重新检测；尚未发送任何评测。");
      // Consume the initial gesture with the first probe, then exercise the
      // same delayed-window case required by later submissions.
      await pause(B.INTERVAL, signal);
      if (signal.aborted) throw new DOMException("已停止弹窗检测；未发送评测。", "AbortError");
      if (!probe()) throw new Error("延时弹窗被拦截：请仅为本站允许弹出式窗口，再点击重新检测；尚未发送任何评测。");
      ready.add(target);
    }
    async function send(item, target, signal) {
      if (signal.aborted) throw new DOMException("已停止后续提交；当前请求仍须核对YOJ记录。", "AbortError");
      if (!ready.has(target)) throw new Error("弹窗检测尚未通过；未发送评测。");
      const form = owner.createElement("form");
      form.method = "post"; form.action = item.submitEndpoint;
      form.target = "_blank"; form.rel = "noopener"; form.acceptCharset = "UTF-8"; form.hidden = true;
      for (const [name, value] of Object.entries(B.formFields(item))) {
        const input = owner.createElement("input");
        input.type = "hidden"; input.name = name; input.value = value; form.append(input);
      }
      owner.body.append(form);
      // Keep the initiating form alive for Safari's native confirmation.
      // At most ten forms; they leave with this document. No response is
      // inferred from submit() returning, and no response window is reused.
      form.submit();
    }
    return Object.freeze({openTarget, prepare, send});
  }
  return Object.freeze({createTransport});
});
