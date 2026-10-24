/* Direct native POSTs, independent response pages. Never reads results or retries. */
(function (root, factory) {
  if (typeof module === "object" && module.exports) module.exports = factory(require("./batch-submit.js"));
  else root.YOJBatchTransport = factory(root.YOJBatch);
})(typeof globalThis !== "undefined" ? globalThis : this, function (B) {
  "use strict";
  function createTransport({document: owner = globalThis.document} = {}) {
    // This is a batch descriptor, NOT a claim that any popup was opened.
    // Native target=_blank avoids preopened/named cross-site window reuse.
    function openTarget() { return Object.freeze({name: "_blank", closed: false}); }
    async function send(item, target, signal) {
      if (signal.aborted) throw new DOMException("已停止后续提交；当前请求仍须核对YOJ记录。", "AbortError");
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
    return Object.freeze({openTarget, send});
  }
  return Object.freeze({createTransport});
});
