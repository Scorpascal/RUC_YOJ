/* Invoked by the user's bookmark on YOJ. All judge requests stay on YOJ. */
(async()=>{
  'use strict';
  const SITE='https://scorpascal.github.io/RUC_YOJ/';
  if(location.hostname!=='yoj.ruc.edu.cn'||location.port||!['http:','https:'].includes(location.protocol)){alert('请先打开题库为本批次生成的 YOJ 页面，再点击此书签。');return;}
  if(window.__yojInPageBatch){window.__yojInPageBatch.show();return;}
  const host=document.createElement('div');host.id='yoj-inpage-panel';host.style.cssText='position:fixed;inset:16px;z-index:2147483647;overflow:auto;background:var(--panel-bg);color:var(--panel-ink);border:2px solid #5f8fff;border-radius:14px;padding:24px;font:16px/1.7 -apple-system,BlinkMacSystemFont,sans-serif;box-sizing:border-box';
  const shadow=host.attachShadow({mode:'open'});document.body.append(host);
  const style=document.createElement('style');style.textContent=':host{--panel-bg:#fff;--panel-ink:#182941;color-scheme:light}button:disabled{opacity:.45;cursor:default!important}@media(prefers-color-scheme:dark){:host{--panel-bg:#172135;--panel-ink:#eef3ff;color-scheme:dark}}';shadow.append(style);
  const title=document.createElement('h2');title.textContent='YOJ 页内批量提交';
  const status=document.createElement('p');status.textContent='正在核验整批公开代码，尚未发送评测。';
  const list=document.createElement('div'),stop=document.createElement('button'),hide=document.createElement('button');
  for(const b of [stop,hide])b.style.cssText='margin:8px;padding:10px 18px;background:#315dca;color:white;border:0;border-radius:8px;font:inherit;cursor:pointer';
  stop.textContent='停止后续提交';hide.textContent='收起面板';
  const note=document.createElement('p');note.textContent='每题至少间隔3秒，逐题核对原站受理记录；最终评测结果仍在YOJ查看。单题权限、模板或题面读取异常将跳过该题；账号状态无法确认或受理不明时停止。停止不会撤回已发送请求，结果不明不自动重试。';
  shadow.append(title,status,list,stop,hide,note);
  const controller=new AbortController();let runner=null,release=()=>{};
  window.__yojInPageBatch={show(){host.hidden=false;}};
  stop.onclick=()=>{controller.abort();runner?.stop();stop.disabled=true;status.textContent='已停止后续提交；当前请求若已发送，请核对YOJ记录。';};
  hide.onclick=()=>{host.hidden=true;};
  addEventListener('pagehide',()=>{controller.abort();runner?.stop();release();try{window.__yojInPageGate?.window.close();}catch(_){}},{once:true});
  async function script(path,key){
    const old=window[key];
    await new Promise((resolve,reject)=>{
      const s=document.createElement('script');s.charset='UTF-8';s.src=SITE+path;let timer;
      const clean=()=>{clearTimeout(timer);controller.signal.removeEventListener('abort',abort);s.onload=s.onerror=null;};
      const abort=()=>{clean();s.remove();reject(new DOMException('已停止','AbortError'));};
      s.onload=()=>{clean();resolve();};s.onerror=()=>{clean();s.remove();reject(Error('核验脚本加载失败，未发送。'));};
      timer=setTimeout(()=>{clean();s.remove();reject(Error('核验脚本加载超时，未发送。'));},30000);
      controller.signal.addEventListener('abort',abort,{once:true});if(controller.signal.aborted){abort();return;}document.head.append(s);
    });
    const value=window[key];if(old===undefined)delete window[key];else window[key]=old;
    if(!value)throw Error('核验脚本不可用，未发送。');return value;
  }
  const normalized=s=>s.replace(/\r\n|\r/g,'\n');
  let request;const problemPages=new Map();
  const parse=text=>new DOMParser().parseFromString(text,'text/html');
  function logged(doc){return !!doc.querySelector('a[href*="/login/logout"]');}
  async function rows(signal){
    const doc=parse((await request('/index.php/index/submissions/index.html?score_min=0',{signal,label:'查询近期提交记录'})).text);
    if(!logged(doc)){
      const login=doc.querySelector('input[type=password], a[href*="/login/index"]');
      throw Error(login?'YOJ 登录已失效，已停止。':'无法确认 YOJ 登录状态，已停止；请在原站核对。');
    }
    const table=[...doc.querySelectorAll('table')].find(t=>{const h=[...t.querySelectorAll('th')].map(e=>e.textContent).join(' ');return ['编号','题目','状态'].every(s=>h.includes(s));});
    if(!table)throw Error('无法确认原站提交列表结构，未继续发送。');
    const dataRows=[...table.querySelectorAll('tr')].filter(tr=>tr.querySelector('td'));
    const result=dataRows.flatMap(tr=>{
      const s=tr.querySelector('a[href*="/submissions/detail/subno/"]')?.getAttribute('href')?.match(/\/subno\/(\d+)/);
      const p=tr.querySelector('a[href*="/problem/detail/pno/"]')?.getAttribute('href')?.match(/\/pno\/(\d+)/);
      if(!s||!p||!tr.querySelector('.status'))throw Error('原站提交记录无法完整解析，未继续发送。');
      return [{submissionNo:Number(s[1]),problemNo:Number(p[1]),status:tr.querySelector('.status').textContent.trim()}];
    });
    const counts=[...doc.body.textContent.matchAll(/(?:^|\D)(\d+)\s*条记录/g)].map(m=>Number(m[1]));
    if(!result.length&&(counts.length!==1||counts[0]!==0))throw Error('无法确认原站空列表，未继续发送。');
    return result;
  }
  try {
    const H=await script('assets/inpage-batch-core.js?v=3','YOJInPageCore');
    request=H.createRequester({origin:location.origin,fetcher:fetch,signal:controller.signal,onRetry:message=>{status.textContent=message;}});
    const fragment=location.hash,batchId=new URLSearchParams(location.search).get('yoj_batch');
    if(batchId&&!/^[a-f0-9]{8}(?:-[a-f0-9]{4}){3}-[a-f0-9]{12}$/.test(batchId))throw Error('批次编号无效。');
    if(!batchId&&!fragment.startsWith('#yoj-batch='))throw Error('缺少本批次清单，请回题库选择题目并打开YOJ。');
    let payload=batchId?null:H.parseHandoff(decodeURIComponent(fragment.slice(11)));
    if(controller.signal.aborted)return;
    if(!logged(document))await rows(controller.signal);
    // A short settling window resolves simultaneous same-origin lease claims.
    // Persistent storage holds only a random owner, times, batch IDs and numbers.
    const gateHandle=window.__yojInPageGate;
    if(!gateHandle?.window||!/^[a-f0-9]{32}$/.test(gateHandle.nonce))throw Error('缺少HTTPS核验窗口，请从保存的书签启动。');
    const nonce=gateHandle.nonce;
    const database=await new Promise((resolve,reject)=>{
      const request=indexedDB.open('yoj-batch-control-v1',1);let settled=false,timer;
      const clean=()=>{clearTimeout(timer);controller.signal.removeEventListener('abort',abort);};
      const fail=message=>{if(settled)return;settled=true;clean();reject(Error(message));};
      const abort=()=>fail('已停止，未发送新的评测。');
      request.onupgradeneeded=()=>{request.result.createObjectStore('leases');};
      request.onsuccess=()=>{if(settled){request.result.close();return;}settled=true;clean();resolve(request.result);};
      request.onerror=()=>fail('无法打开本机运行记录，未发送。');request.onblocked=()=>fail('本机运行记录正在升级，请关闭旧批次页面后再试。');
      timer=setTimeout(()=>fail('本机运行记录加载超时，未发送。'),30000);
      controller.signal.addEventListener('abort',abort,{once:true});if(controller.signal.aborted)abort();
    });
    function lease(action){return new Promise((resolve,reject)=>{
      const tx=database.transaction('leases','readwrite'),store=tx.objectStore('leases'),get=store.get('active');let reason='';
      const timer=setTimeout(()=>{reason='本机运行记录操作超时，已停止。';try{tx.abort();}catch(_){}},10000);
      get.onsuccess=()=>{
        const current=get.result;
        if(current&&(!/^[a-f0-9]{32}$/.test(current.owner||'')||!Number.isFinite(current.at))){reason='运行记录损坏，未发送。';tx.abort();return;}
        if(action==='release'){if(current?.owner===nonce)store.delete('active');return;}
        if(action==='claim'?(current&&Date.now()-current.at<120000):(current?.owner!==nonce)){
          reason=action==='claim'?'另一个YOJ批次正在运行，请先停止或等待完成。':'运行控制权已变化，已停止，勿重发。';tx.abort();return;
        }
        store.put({owner:nonce,at:Date.now()},'active');
      };
      tx.oncomplete=()=>{clearTimeout(timer);resolve();};tx.onabort=tx.onerror=()=>{clearTimeout(timer);reject(Error(reason||'本机运行记录不可用，已停止。'));};
    });}
    let heartbeat;
    release=async()=>{clearInterval(heartbeat);try{await lease('release');}catch(_){}database.close();};
    await lease('claim');if(controller.signal.aborted)return;
    const guard=()=>lease('renew');
    heartbeat=setInterval(()=>{guard().catch(()=>{controller.abort();runner?.stop();});},2000);
    const B=await script('assets/batch-submit.js','YOJBatch'),previousB=window.YOJBatch;
    let S;window.YOJBatch=B;
    try{S=await script('assets/batch-session.js','YOJBatchSession');}finally{if(previousB===undefined)delete window.YOJBatch;else window.YOJBatch=previousB;}
    if(controller.signal.aborted)return;await guard();
    const items=await new Promise((resolve,reject)=>{
      const gate=gateHandle.window;let timer,hello;
      function clean(){clearTimeout(timer);clearInterval(hello);removeEventListener('message',receive);controller.signal.removeEventListener('abort',abort);try{gate.close();}catch(_){}}
      function abort(){clean();reject(new DOMException('已停止','AbortError'));}
      function receive(event){
        if(event.origin!==new URL(SITE).origin||event.source!==gate||event.data?.nonce!==nonce)return;
        if(event.data.type==='gate-ready')gate.postMessage({type:'verify',nonce,...(batchId?{batchId}:{payload})},new URL(SITE).origin);
        else if(event.data.type==='verified'){clean();try{
          if(batchId){payload=H.parseHandoff(JSON.stringify(event.data.payload));if(payload.id!==batchId)throw Error('核验返回了其他批次，未发送。');}
          resolve(H.matchItems(payload,event.data.items));}catch(error){reject(error);}}
        else if(event.data.type==='failed'){clean();reject(Error('公开代码核验失败或已变化，未发送；请返回题库重新核对。'));}
      }
      addEventListener('message',receive);controller.signal.addEventListener('abort',abort,{once:true});
      timer=setTimeout(()=>{clean();reject(Error('公开代码核验超时，未发送。'));},60000);
      const ping=()=>{try{gate.postMessage({type:'hello',nonce},new URL(SITE).origin);}catch(_){}};
      if(controller.signal.aborted){abort();return;}hello=setInterval(ping,500);ping();
    });
    if(controller.signal.aborted)return;
    const ledger=S.createLedger(localStorage,payload.id,payload.items.map(i=>i.problemNo));ledger.claimMode('inpage');
    const cells=new Map();for(const item of items){const p=document.createElement('p');p.textContent='#'+item.problemNo+' '+item.title+'：等待原站检查';list.append(p);cells.set(item.problemNo,p);}
    runner=H.createRunner(items,{
      guard,attempted:()=>ledger.attempted(),remember:n=>ledger.remember(n),
      async preflightItem(item,signal){
          const unavailable=message=>H.itemUnavailable(message);
          const read=async(path,options)=>{
            try{return await request(path,{...options,signal});}catch(error){
              if(signal.aborted||error.name==='AbortError')throw error;
              throw unavailable(error.message);
            }
          };
          status.textContent='正在检查 #'+item.problemNo+' 的原站题面、语言和权限；尚未发送。';
          const page=await read(new URL(item.problemUrl).pathname,{signal,problem:true,label:'题面检查 #'+item.problemNo}),doc=parse(page.text);
          problemPages.set(item.problemNo,page.url);
          // Error/permission pages may omit the navigation while the account is valid.
          if(!logged(doc))await rows(signal);
          const form=doc.querySelector('#submit_code');
          if(item.originalSubmissionKind==='PARTIAL_CODE'||new URL(page.url).pathname.includes('/detailtk/')){
            const heading=[...doc.querySelectorAll('h1,h2,h3')].some(h=>new RegExp('#\\s*'+item.problemNo+'(?:\\D|$)').test(h.textContent));
            if(!heading||(doc.querySelector('[name=pid]')&&doc.querySelector('[name=pid]').value!==String(item.problemNo))||!doc.querySelector('#btn-submit')||![...doc.querySelectorAll('#compiler_select option')].some(e=>e.value===item.language))throw unavailable('当前题面、语言或权限不可确认，本题未发送。');
            let template;try{template=JSON.parse((await read('/index.php/index/problem/gettkcode.html',{data:{pno:String(item.problemNo)},signal,referrer:page.url,label:'读取填空模板 #'+item.problemNo})).text);}catch(error){if(signal.aborted||error.name==='AbortError')throw error;throw unavailable('模板检查：'+error.message);}
            if(!template||typeof template!=='object'||String(template.status)!=='1'||typeof template.content!=='string'||!template.content.includes('____qcodep____'))throw unavailable('当前填空模板不可用，本题未发送。');
            if(!H.matchesTemplate(template.content,item.code))throw unavailable('当前单填空模板与已发布完整程序不匹配，本题未发送。');
          }else if(!form||form.querySelector('[name="pid"]')?.value!==String(item.problemNo)||!form.querySelector('[name="code"]')||![...doc.querySelectorAll('[data-value]')].some(e=>e.getAttribute('data-value')===item.language)){
            throw unavailable('题号、语言、提交表单或课程/考试权限不可确认，本题未发送。');
          }
      },
      async baseline(signal){return Math.max(0,...(await rows(signal)).map(r=>r.submissionNo));},
      async post(item,signal){
        const {text}=await request(new URL(item.submitEndpoint).pathname,{data:{pid:String(item.problemNo),language:item.language,code:item.code},signal,referrer:problemPages.get(item.problemNo),judge:true,label:'提交 #'+item.problemNo});
        if(text===null)return; // Redirect is not acceptance; receipt checks follow.
        let response;try{response=JSON.parse(text.replace(/^\ufeff/,''));}catch(_){throw Error('YOJ 未返回可确认的受理响应，已停止，请核对记录。');}
        if(String(response?.status)!=='1')throw Error('YOJ 拒绝或未确认本题，已停止，请查看原站记录。');
      },
      async receipt(item,baseline,signal){
        for(let i=0;i<20;i++){
          if(signal.aborted)throw new DOMException('已停止','AbortError');
          const fresh=(await rows(signal)).filter(r=>r.submissionNo>baseline&&r.problemNo===item.problemNo);
          if(fresh.length>1)throw Error('同时出现多条同题记录，无法唯一确认本次受理，已停止。');
          if(fresh.length===1){
            let recovered;try{recovered=JSON.parse((await request('/index.php/index/submissions/getcode.html',{data:{subid:String(fresh[0].submissionNo)},signal,label:'回收新增记录源码 #'+item.problemNo})).text);}catch(error){throw Error('#'+item.problemNo+' 源码回收：'+error.message);}
            if(String(recovered?.status)==='1'&&typeof recovered.code==='string'){
              if(normalized(recovered.code)!==normalized(item.code))throw Error('新增记录源码不匹配，已停止，请核对原站。');
              return fresh[0];
            }
          }
          await H.pause(1500,signal);
        }
        throw Error('未找到可确认的新提交记录，已停止，勿自动重发。');
      },
      onChange(snapshot){
        for(const row of snapshot.rows){const cell=cells.get(row.problemNo);cell.textContent='#'+row.problemNo+'：'+({BLOCKED:'已跳过，未发送 · '+row.reason,WAITING:'尚未发送',UNCERTAIN:'已尝试，正在核对受理记录',PREVIOUS:'本批次已尝试，已跳过，勿重发',RECEIVED:'原站已受理 #'+row.receipt?.submissionNo+' · '+row.receipt?.status})[row.state];}
        const counts=state=>snapshot.rows.filter(r=>r.state===state).length;
        status.textContent=({PREPARING:'正在检查整批原站题号、语言和权限，尚未发送。',RUNNING:'正在逐题发送并核对原站记录；无需逐题确认。',DONE:'本批处理结束：本轮已受理 '+counts('RECEIVED')+' 题，检查未通过跳过 '+counts('BLOCKED')+' 题，此批此前已尝试 '+counts('PREVIOUS')+' 题。'+(counts('BLOCKED')?' 跳过原因见各题；恢复后刷新本页再点书签，只重查未发送题。':'')+' 最终评测结果请在YOJ查看。',STOPPED:(snapshot.rows.some(r=>r.state==='UNCERTAIN')?'已停止后续提交。结果不明的题目请核对原站，不会自动重发。':'本轮已停止，尚未发送的题目仍未发送。')+' 刷新本页后再点书签，可重新检查未发送题；已尝试题不会重发。'})[snapshot.phase]+(snapshot.detail?' '+snapshot.detail:'');
      }
    });
    await runner.start();

  }catch(error){status.textContent=error.name==='AbortError'?'已停止；请核对原站记录。':error.message;}
  finally{await release();stop.disabled=true;try{window.__yojInPageGate?.window.close();}catch(_){}delete window.__yojInPageGate;}
})();
