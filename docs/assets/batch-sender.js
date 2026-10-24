/* One real document owns one checked native POST; no credentials or storage. */
(function (root, factory) {
  if (typeof module === "object" && module.exports) module.exports = factory(require("./batch-submit.js"));
  else { root.YOJBatchSender = factory(root.YOJBatch); root.YOJBatchSender.bootstrap(root); }
})(typeof globalThis !== "undefined" ? globalThis : this, function (B) {
  "use strict";
  const keys = ["problemNo", "language", "codeSha256", "problemUrl", "submitEndpoint"];
  const validTicket = value => /^[a-f0-9]{8}(?:-[a-f0-9]{4}){3}-[a-f0-9]{12}$/.test(value || "");
  function createSender({ticket, instance = globalThis.crypto.randomUUID(), channel, loadItem, submit, onState = () => {}}) {
    if (!validTicket(ticket) || !validTicket(instance)) throw new Error("无效发送页，请从批量页重新打开。");
    let state = "READY", item = null;
    function emit(type, extra = {}) { channel.postMessage({ticket, instance, type, ...extra}); }
    function fail() { state = "FAILED"; onState("FAILED"); emit("failed"); }
    channel.onmessage = async ({data: m}) => {
      if (!m || m.ticket !== ticket || m.instance !== instance || ["FAILED", "LEFT", "SENT"].includes(state)) return;
      if (m.type === "cancel") { state = "FAILED"; onState("CANCELLED"); return; }
      if (m.type === "prepare" && state === "READY") {
        state = "LOADING";
        try {
          const expected = m.expected;
          if (!expected || !Number.isSafeInteger(expected.problemNo) || expected.problemNo < 1) throw Error();
          const loaded = await loadItem(expected.problemNo);
          if (state !== "LOADING") return;
          if (!loaded || keys.some(k => loaded[k] !== expected[k])) throw Error();
          item = loaded; state = "PREPARED"; onState(state, item); emit("prepared", {problemNo: item.problemNo});
        } catch (_) { if (state === "LOADING") fail(); }
      } else if (m.type === "send") {
        if (state !== "PREPARED" || m.problemNo !== item.problemNo) { fail(); return; }
        state = "SENT"; onState(state, item); emit("sending", {problemNo: item.problemNo});
        try { submit(item); } catch (_) { fail(); }
      }
    };
    emit("ready");
    return Object.freeze({left() {
      // pagehide is a departure signal, not proof of receipt or acceptance.
      // In Safari the opener's WindowProxy can be severed during navigation.
      if (state === "SENT") { state = "LEFT"; emit("left", {problemNo: item.problemNo}); }
    }});
  }
  function nativePost(doc, item) {
    doc.defaultView.opener = null;
    const form = doc.createElement("form");
    form.method = "post"; form.action = item.submitEndpoint; form.target = "_self";
    form.acceptCharset = "UTF-8"; form.hidden = true;
    for (const [name, value] of Object.entries(B.formFields(item))) {
      const input = doc.createElement("input");
      input.type = "hidden"; input.name = name; input.value = value; form.append(input);
    }
    doc.body.append(form); form.submit();
  }
  function bootstrap(win) {
    const note = win.document.getElementById("sender-status");
    const ticket = win.location.hash.slice(1);
    if (!validTicket(ticket) || !win.BroadcastChannel) {
      note.textContent = "请从批量提交页打开此发送页；没有发送评测。"; return;
    }
    win.opener = null;
    const channel = new win.BroadcastChannel("yoj-sender-v1:" + ticket);
    async function json(path) {
      const response = await win.fetch(path, {cache: "no-store", credentials: "omit"});
      if (!response.ok) throw Error("公开清单不可用");
      return response.json();
    }
    const sender = createSender({ticket, channel,
      async loadItem(problemNo) {
        const [manifest, catalog] = await Promise.all([json("data/quick-submit.json"), json("data/catalog.json")]);
        return (await B.prepareBatch([problemNo], manifest, catalog, win.fetch.bind(win), win.crypto))[0];
      },
      submit: item => nativePost(win.document, item),
      onState(state, item) {
        if (item) win.document.title = `YOJ #${item.problemNo} 发送页`;
        note.textContent = ({PREPARED: `#${item?.problemNo} 源码与发布证据已核验，等待批量页发送指令。`,
          SENT: `正在发送 #${item?.problemNo}。如出现浏览器安全确认，请自行发送或取消。取消后请回批量页停止。`,
          FAILED: "发送页校验或发送失败，已停止；请回批量页查看并核对YOJ记录，不要重复发送。",
          CANCELLED: "已取消，本发送页不会发起评测。"})[state] || "正在核验公开代码。";
      }
    });
    win.addEventListener("pagehide", () => sender.left());
  }
  return Object.freeze({createSender, nativePost, bootstrap, validTicket});
});
