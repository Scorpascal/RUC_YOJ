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
  const approvalKey=id=>'yoj-batch-approved-v1:'+id;
  function saveApproved(storage,payload,now=Date.now()){
    const p=parseHandoff(JSON.stringify(payload)),previous=storage.getItem(approvalKey(p.id));
    if(previous!==null){const row=JSON.parse(previous),old=parseHandoff(JSON.stringify(row.payload));
      if(old.id!==p.id)throw Error('批次确认不匹配，请新建批次。');matchItems(p,old.items);}
    storage.setItem(approvalKey(p.id),JSON.stringify({payload:p,expiresAt:now+1800000}));
  }
  function loadApproved(storage,id,now=Date.now()){
    if(!/^[a-f0-9]{8}(?:-[a-f0-9]{4}){3}-[a-f0-9]{12}$/.test(id||''))throw Error('批次编号无效。');
    const row=JSON.parse(storage.getItem(approvalKey(id))||'null');
    if(!row||!Number.isFinite(row.expiresAt)||row.expiresAt<=now||row.expiresAt>now+1800000)throw Error('批次确认已过期或不可读取，请回题库重新确认。');
    const p=parseHandoff(JSON.stringify(row.payload));if(p.id!==id)throw Error('批次确认不匹配。');return p;
  }
  function matchItems(payload,items){if(!Array.isArray(items)||items.length!==payload.items.length||items.some((e,i)=>keys.some(k=>e[k]!==payload.items[i][k])))throw Error('公开代码或题目身份已变化，请回题库重新核对。');return items;}
  function pause(ms,signal){return new Promise(resolve=>{const done=()=>{clearTimeout(timer);signal.removeEventListener('abort',done);resolve();};const timer=setTimeout(done,ms);signal.addEventListener('abort',done,{once:true});if(signal.aborted)done();});}
  function matchesTemplate(template,code){
    if(typeof template!=='string'||typeof code!=='string')return false;
    const normalize=s=>s.replace(/\r\n|\r/g,'\n'),parts=normalize(template).split('____qcodep____'),source=normalize(code);
    return parts.length===2&&!source.includes('____qcodep____')&&source.startsWith(parts[0])&&source.endsWith(parts[1])&&
      source.slice(parts[0].length,source.length-parts[1].length).trim().length>0;
  }
  function createRequester({origin,fetcher,signal:outerSignal,onRetry=()=>{},pause:wait=pause,timeoutMs=30000}){
    const problemPath=/^\/index\.php\/index\/problem\/(detail|detailtk)\/pno\/[1-9]\d*(?:\/eno\/[1-9]\d*)?\.html$/;
    const abort=()=>new DOMException('已停止','AbortError');
    return async function request(path,{data,signal=outerSignal,referrer,judge=false,problem=false,label='YOJ 检查'}={}){
      let url=new URL(path,origin);
      if(url.origin!==origin||url.username||url.password||url.hash)throw Error(label+'：原站地址不一致。');
      if(problem&&(data||url.search||!problemPath.test(url.pathname)))throw Error(label+'：题面地址无效。');
      const readOnly=!judge&&(data?['/index.php/index/problem/gettkcode.html','/index.php/index/submissions/getcode.html'].includes(url.pathname):problemPath.test(url.pathname)||url.pathname==='/index.php/index/submissions/index.html');
      let canonical=false;
      for(let attempt=1;;attempt++){
        if(signal.aborted)throw abort();
        const timeout=new AbortController();let timedOut=false;
        const stop=()=>timeout.abort();signal.addEventListener('abort',stop,{once:true});
        const timer=setTimeout(()=>{timedOut=true;timeout.abort();},timeoutMs);
        let failure;
        try{
          const response=await fetcher(url.href,{method:data?'POST':'GET',credentials:'same-origin',cache:'no-store',signal:timeout.signal,redirect:'manual',
            ...(referrer?{referrer:new URL(referrer,origin).href,referrerPolicy:'same-origin'}:{}),
            headers:data?{'Content-Type':'application/x-www-form-urlencoded;charset=UTF-8','X-Requested-With':'XMLHttpRequest'}:{},body:data?new URLSearchParams(data):undefined});
          if(response.type==='opaqueredirect'||(response.status>=300&&response.status<400)){
            if(judge)return {text:null,url:url.href};
            // Never follow an unknown Location. Probe only the same approved
            // problem/context's fixed canonical route, then validate its form.
            if(problem&&!canonical&&url.pathname.includes('/detail/')){
              canonical=true;url=new URL(url.href.replace('/detail/','/detailtk/'));attempt=0;continue;
            }
            throw Error(label+'：原站出现未支持的跳转，未继续发送。');
          }
          if(!response.ok){const e=Error('HTTP '+response.status);e.retryable=[408,429,500,502,503,504].includes(response.status);throw e;}
          if(new URL(response.url).href!==url.href)throw Error(label+'：原站返回地址不一致。');
          return {text:await response.text(),url:url.href};
        }catch(error){
          if(signal.aborted)throw abort();
          const transient=timedOut||error.name==='TypeError'||error.retryable===true;
          failure=Error(label+'：'+(timedOut?'请求超时':error.name==='TypeError'?'网络读取失败（Load failed）':error.message)+(readOnly?'。尚未确认此步骤。':'。请核对原站记录，勿重发。'));
          if(!readOnly||!transient||attempt>=3)throw failure;
        }finally{clearTimeout(timer);signal.removeEventListener('abort',stop);}
        onRetry(label+'：网络暂不可用，正在安全重查（'+(attempt+1)+'/3）。');
        await wait(attempt*1000,signal);
      }
    };
  }
  function createRunner(items,io){
    const controller=new AbortController(),rows=items.map(e=>({problemNo:e.problemNo,state:'WAITING'}));let phase='READY',detail='',used=false;
    const snapshot=()=>({phase,detail,rows:rows.map(r=>({...r}))}),notify=()=>io.onChange?.(snapshot());
    function stop(){controller.abort();if(phase==='RUNNING'||phase==='PREPARING'){phase='STOPPED';notify();}}
    async function start(){
      if(used)return;used=true;phase='PREPARING';notify();
      try {
        await io.guard();if(controller.signal.aborted)return;
        const attempted=io.attempted(),pending=items.filter((item,i)=>{if(attempted.has(item.problemNo)){rows[i].state='PREVIOUS';return false;}return true;});notify();
        if(pending.length)await io.preflight(pending,controller.signal);if(controller.signal.aborted)return;
        phase='RUNNING';notify();let posted=false;
        for(let i=0;i<items.length;i++) {
          const item=items[i],row=rows[i];
          if(io.attempted().has(item.problemNo)){row.state='PREVIOUS';notify();continue;}
          if(posted)await (io.pause||pause)(3000,controller.signal);
          if(controller.signal.aborted)return;
          await io.guard();if(controller.signal.aborted)return;const baseline=await io.baseline(controller.signal);if(controller.signal.aborted)return;
          await io.guard();if(controller.signal.aborted)return;io.remember(item.problemNo);row.state='UNCERTAIN';notify();
          // A redirect/network failure can occur after YOJ accepted the POST.
          // Reconcile once through read-only evidence; never repeat the POST.
          try{await io.post(item,controller.signal);}catch(error){detail='提交响应未确认，正在核对原站记录。';notify();}
          posted=true;if(controller.signal.aborted)return;
          const receipt=await io.receipt(item,baseline,controller.signal);
          if(!receipt||!Number.isSafeInteger(receipt.submissionNo)||receipt.submissionNo<=baseline)throw Error('原站受理结果不明，已停止；请核对记录，勿重发。');
          detail='';row.receipt=receipt;row.state='RECEIVED';notify();if(controller.signal.aborted)return;
        }
        phase='DONE';notify();
      }catch(error){phase='STOPPED';detail=error.message||'结果不明，已停止。';notify();}
      finally{if(controller.signal.aborted){phase='STOPPED';notify();}}
    }
    return Object.freeze({start,stop,snapshot});
  }
  return Object.freeze({parseHandoff,makeHandoff,saveApproved,loadApproved,matchItems,createRunner,pause,matchesTemplate,createRequester});
});
