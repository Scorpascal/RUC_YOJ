/* Shared public handoff and receipt-driven queue; no login or code persistence. */
(function(root,factory){const api=factory();if(typeof module==='object'&&module.exports)module.exports=api;else root.YOJInPageCore=api;})(typeof globalThis!=='undefined'?globalThis:this,function(){
  'use strict';
  const keys=['problemNo','codeSha256','language','problemUrl','submitEndpoint'];
  function parseHandoff(text) {
    if(typeof text!=='string'||text.length>20000)throw Error('批次信息无效。');
    const p=JSON.parse(text);
    if(p?.version!==1||!/^[a-f0-9]{8}(?:-[a-f0-9]{4}){3}-[a-f0-9]{12}$/.test(p.id||'')||!Array.isArray(p.items)||p.items.length<1||p.items.length>10)throw Error('批次编号或数量无效。');
    const seen=new Set();
    for(const e of p.items) {
      if(!e||Object.keys(e).length!==keys.length||!keys.every(k=>Object.hasOwn(e,k))||!Number.isSafeInteger(e.problemNo)||e.problemNo<1||seen.has(e.problemNo)||!/^\w[\w+.-]{0,39}$/.test(e.language||'')||!/^[a-f0-9]{64}$/.test(e.codeSha256||''))throw Error('题目身份无效或重复。');
      seen.add(e.problemNo);
      for(const key of ['problemUrl','submitEndpoint']) {const u=new URL(e[key]);if(u.hostname!=='yoj.ruc.edu.cn'||!['http:','https:'].includes(u.protocol)||u.port||u.username||u.password||u.hash||u.search)throw Error('题目地址无效。');}
    }
    return p;
  }
  function makeHandoff(id,items){return parseHandoff(JSON.stringify({version:1,id,items:items.map(e=>Object.fromEntries(keys.map(k=>[k,e[k]])))}));}
  function matchItems(payload,items){if(!Array.isArray(items)||items.length!==payload.items.length||items.some((e,i)=>keys.some(k=>e[k]!==payload.items[i][k])))throw Error('公开代码或题目身份已变化，请回题库重新核对。');return items;}
  function pause(ms,signal){return new Promise(resolve=>{const done=()=>{clearTimeout(timer);signal.removeEventListener('abort',done);resolve();};const timer=setTimeout(done,ms);signal.addEventListener('abort',done,{once:true});if(signal.aborted)done();});}
  function createRunner(items,io){
    const controller=new AbortController(),rows=items.map(e=>({problemNo:e.problemNo,state:'WAITING'}));let phase='READY',detail='',used=false;
    const snapshot=()=>({phase,detail,rows:rows.map(r=>({...r}))}),notify=()=>io.onChange?.(snapshot());
    function stop(){controller.abort();if(phase==='RUNNING'||phase==='PREPARING'){phase='STOPPED';notify();}}
    async function start(){
      if(used)return;used=true;phase='PREPARING';notify();
      try {
        await io.guard();if(controller.signal.aborted)return;await io.preflight(items,controller.signal);if(controller.signal.aborted)return;
        phase='RUNNING';notify();let posted=false;
        for(let i=0;i<items.length;i++) {
          const item=items[i],row=rows[i];
          if(io.attempted().has(item.problemNo)){row.state='PREVIOUS';notify();continue;}
          if(posted)await (io.pause||pause)(3000,controller.signal);
          if(controller.signal.aborted)return;
          await io.guard();if(controller.signal.aborted)return;const baseline=await io.baseline(controller.signal);if(controller.signal.aborted)return;
          await io.guard();if(controller.signal.aborted)return;io.remember(item.problemNo);row.state='UNCERTAIN';notify();
          await io.post(item,controller.signal);posted=true;if(controller.signal.aborted)return;
          const receipt=await io.receipt(item,baseline,controller.signal);
          if(!receipt||!Number.isSafeInteger(receipt.submissionNo)||receipt.submissionNo<=baseline)throw Error('原站受理结果不明，已停止；请核对记录，勿重发。');
          row.receipt=receipt;row.state='RECEIVED';notify();if(controller.signal.aborted)return;
        }
        phase='DONE';notify();
      }catch(error){phase='STOPPED';detail=error.message||'结果不明，已停止。';notify();}
      finally{if(controller.signal.aborted){phase='STOPPED';notify();}}
    }
    return Object.freeze({start,stop,snapshot});
  }
  return Object.freeze({parseHandoff,makeHandoff,matchItems,createRunner,pause});
});
