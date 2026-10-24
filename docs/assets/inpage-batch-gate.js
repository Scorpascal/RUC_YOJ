/* HTTPS top-level helper keeps WebCrypto checks intact even when YOJ itself is HTTP. */
(() => {
  'use strict';
  const nonce=location.hash.slice(1);let used=false;
  const caller=opener;
  if(!caller||parent!==window||!/^[a-f0-9]{32}$/.test(nonce))return;
  addEventListener('message',async event=>{
    if(used||event.source!==caller||!['http://yoj.ruc.edu.cn','https://yoj.ruc.edu.cn'].includes(event.origin)||event.data?.nonce!==nonce)return;
    if(event.data?.type==='hello'){caller.postMessage({type:'gate-ready',nonce},event.origin);return;}
    if(event.data?.type!=='verify')return;
    used=true;
    const reply=data=>caller.postMessage({nonce,...data},event.origin);
    try {
      const p=event.data.batchId?YOJInPageCore.loadApproved(localStorage,event.data.batchId):YOJInPageCore.parseHandoff(JSON.stringify(event.data.payload));
      const read=async path=>{const r=await fetch(path,{cache:'no-store',credentials:'omit'});if(!r.ok)throw Error('公开清单不可用');return r.json();};
      const [manifest,catalog]=await Promise.all([read('data/quick-submit.json'),read('data/catalog.json')]);
      const items=await YOJBatch.prepareBatch(p.items.map(e=>e.problemNo),manifest,catalog,fetch,crypto);
      YOJInPageCore.matchItems(p,items);
      reply({type:'verified',payload:p,items:items.map(e=>({...p.items.find(i=>i.problemNo===e.problemNo),code:e.code,title:e.title,originalSubmissionKind:e.originalSubmissionKind||'STANDARD_CODE'}))});
    }catch(_){reply({type:'failed'});}
  });
  caller.postMessage({type:'gate-ready',nonce},'*'); // Public nonce only; code replies target the checked origin.
})();
