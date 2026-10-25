/* One reusable bookmark and approval handoff for one or up to ten problems. */
(function(root,factory){if(typeof module==='object'&&module.exports)module.exports=factory(require('./inpage-batch-core.js'));else root.YOJInPageEntry=factory(root.YOJInPageCore);})(typeof globalThis!=='undefined'?globalThis:this,function(H){
  'use strict';
  const loader = function () {
      if (location.hostname !== "yoj.ruc.edu.cn" || location.port || !["http:","https:"].includes(location.protocol)) { alert("请先打开本批次的 YOJ 页面，再点此书签。"); return; }
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

  function bookmarkHref(){return 'javascript:'+encodeURIComponent('('+loader.toString()+')();void 0');}
  async function approveAndOpen({id,items,entry,ledger,storage,open}){
    const payload=H.makeHandoff(id,items);
    // Reserve the window synchronously in the user's click, before any await.
    const target=open('about:blank','_blank');
    if(!target)throw Error('YOJ窗口被阻止，尚未发送；请允许本次窗口。');
    try{
      target.opener=null;
      await entry.run('inpage',()=>{
        if(ledger.attempted().size||entry.attempted().size)throw Error('本次已有提交尝试，请先核对原站记录，不会切换方式重发。');
        ledger.claimMode('inpage');H.saveApproved(storage,payload);
        target.location.href='http://yoj.ruc.edu.cn/index.php/index/submissions/index.html?score_min=0&yoj_batch='+payload.id;
      });
      return payload;
    }catch(error){try{target.close();}catch(_){}throw error;}
  }
  return Object.freeze({bookmarkHref,approveAndOpen});
});
