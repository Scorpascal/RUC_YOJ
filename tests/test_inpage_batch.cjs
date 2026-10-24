const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const H=fs.existsSync('docs/assets/inpage-batch-core.js')?require('../docs/assets/inpage-batch-core.js'):{};
const id='11111111-1111-4111-8111-111111111111';
const rows=[8,9].map(problemNo=>({problemNo,codeSha256:'a'.repeat(64),language:'cpp',problemUrl:`http://yoj.ruc.edu.cn/index.php/index/problem/detail/pno/${problemNo}.html`,submitEndpoint:'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit.html',code:'int main(){}'}));
function fixture(options={}) {
 assert.equal(typeof H.createRunner,'function');
 const posts=[],remembered=[],states=[];let clock=0;
 const runner=H.createRunner(rows,{attempted:()=>new Set(),remember:n=>remembered.push(n),guard:()=>{},preflightItem:async()=>{},baseline:async()=>100,
  post:async item=>posts.push({n:item.problemNo,at:clock}),receipt:async(item)=>({submissionNo:100+item.problemNo,problemNo:item.problemNo,status:'Accepted'}),
  pause:async ms=>{clock+=ms;},onChange:s=>states.push(s),...options});
 return {runner,posts,remembered,states};
}
test('handoff keeps exact approved public identity and rejects malformed or changed items',()=>{
 assert.equal(typeof H.makeHandoff,'function');const p=H.makeHandoff(id,rows);assert.deepEqual(H.parseHandoff(JSON.stringify(p)).items,p.items);
 assert.throws(()=>H.parseHandoff(JSON.stringify({...p,items:[p.items[0],p.items[0]]})));
 assert.throws(()=>H.matchItems(p,[{...rows[0],codeSha256:'b'.repeat(64)},rows[1]]));
 assert.equal(JSON.stringify(p).includes('int main'),false);
});
test('one start submits entire batch with receipts and pacing; duplicate start never resends',async()=>{
 const f=fixture();await f.runner.start();await f.runner.start();assert.deepEqual(f.remembered,[8,9]);assert.equal(f.posts.length,2);
 assert.ok(f.posts[1].at-f.posts[0].at>=3000);assert.equal(f.runner.snapshot().phase,'DONE');
 assert.deepEqual(f.runner.snapshot().rows.map(r=>r.receipt.submissionNo),[108,109]);
});
test('missing receipt stops without retry; failed POST response reconciles real evidence once',async()=>{
 const f=fixture({receipt:async()=>{throw Error('unknown');}});await f.runner.start();await f.runner.start();
 assert.deepEqual(f.remembered,[8]);assert.equal(f.runner.snapshot().phase,'STOPPED');
 assert.deepEqual(f.runner.snapshot().rows.map(r=>r.state),['UNCERTAIN','WAITING']);
 let attempts=0;const g=fixture({post:async()=>{attempts++;throw Error('response lost');}});await g.runner.start();
 assert.equal(attempts,2);assert.equal(g.runner.snapshot().phase,'DONE');
 const h=fixture({post:async()=>{throw Error('network');},receipt:async()=>{throw Error('no evidence');}});
 await h.runner.start();assert.equal(h.runner.snapshot().phase,'STOPPED');assert.deepEqual(h.remembered,[8]);
});
test('Stop during async preflight or polling never sends another item',async()=>{
 let release;const f=fixture({preflightItem:()=>new Promise(r=>{release=r;})});const pending=f.runner.start();await new Promise(r=>setImmediate(r));f.runner.stop();release();await pending;
 assert.equal(f.posts.length,0);assert.equal(f.remembered.length,0);
 const g=fixture({receipt:async()=>{g.runner.stop();return {submissionNo:108};}});await g.runner.start();assert.equal(g.posts.length,1);
});
test('prior attempts skipped; storage failure and lost lease fail before any POST',async()=>{
 const f=fixture({attempted:()=>new Set([8])});await f.runner.start();assert.deepEqual(f.posts.map(p=>p.n),[9]);
 for(const options of [{remember(){throw Error('storage');}},{guard(){throw Error('lease');}}]){const g=fixture(options);await g.runner.start();assert.equal(g.posts.length,0);}
});
test('batch execution mode persists and cannot switch silently',()=>{
 const S=require('../docs/assets/batch-session.js'),data=new Map(),storage={getItem:k=>data.get(k)||null,setItem:(k,v)=>data.set(k,v)};
 const ledger=S.createLedger(storage,id,[8,9]);ledger.claimMode('inpage');
 const resumed=S.createLedger(storage,id,[8,9],{resume:true});assert.equal(resumed.mode(),'inpage');assert.throws(()=>resumed.claimMode('cross'));
 resumed.claimMode('inpage');assert.deepEqual([...resumed.attempted()],[]);
});
test('shared entry lock prevents simultaneous tabs, mode switching, and same-batch resends',async()=>{
 const E=require('../docs/assets/batch-entry.js'),data=new Map(),held=new Set();
 const storage={getItem:k=>data.get(k)??null,setItem:(k,v)=>data.set(k,v)};
 const locks={async request(key,options,fn){if(held.has(key))return fn(null);held.add(key);try{return await fn({name:key});}finally{held.delete(key);}}};
 const a=E.create(storage,locks,id,[8,9]),b=E.create(storage,locks,id,[8,9]);let release;
 const running=a.run('cross',async()=>{a.remember(8);await new Promise(r=>release=r);});
 await assert.rejects(b.run('inpage',()=>assert.fail('must not run')),/另一个页面/);
 release();await running;assert.deepEqual([...b.attempted()],[8]);
 await assert.rejects(b.run('inpage',()=>{}),/另一执行方式/);
 await b.run('cross',()=>{assert.throws(()=>b.remember(8),/已尝试/);b.remember(9);});
 const next=E.create(storage,locks,'22222222-2222-4222-8222-222222222222',[8,9]);
 await next.run('inpage',()=>{});assert.equal(next.mode(),'inpage');
 assert.throws(()=>E.create(storage,locks,id,[8]).mode(),/不一致/);
 await assert.rejects(E.create(storage,null,id,[8,9]).run('cross',()=>{}),/跨页/);
});
test('short handoff reads only its unexpired exact public approval',()=>{
 const data=new Map(),storage={getItem:k=>data.get(k)||null,setItem:(k,v)=>data.set(k,v)};
 const p=H.makeHandoff(id,rows);H.saveApproved(storage,p,1000);assert.deepEqual(H.loadApproved(storage,id,2000),p);
 assert.throws(()=>H.loadApproved(storage,id,1801000),/过期/);assert.throws(()=>H.loadApproved(storage,'other',2000));
 assert.equal([...data.values()].join('').includes('int main'),false);
 const original=data.get('yoj-batch-approved-v1:'+id);const changed=JSON.parse(JSON.stringify(p));changed.items[0].codeSha256='b'.repeat(64);assert.throws(()=>H.saveApproved(storage,changed,2000),/变化/);assert.equal(data.get('yoj-batch-approved-v1:'+id),original);H.saveApproved(storage,p,3000);assert.deepEqual(H.loadApproved(storage,id,4000),p);
 const key='yoj-batch-approved-v1:'+id;const row=JSON.parse(data.get(key));row.payload.id='22222222-2222-4222-8222-222222222222';data.set(key,JSON.stringify(row));assert.throws(()=>H.loadApproved(storage,id,4000),/不匹配/);
});

// Regression: Safari rejects detail -> detailtk redirects as "Load failed".
function response(url,text='ok',status=200,type='basic'){return {url,status,type,ok:status>=200&&status<300,text:async()=>text};}
const origin='http://yoj.ruc.edu.cn';
test('only the same problem and eno canonical read is probed after a detail redirect',async()=>{
 const seen=[];const req=H.createRequester({origin,signal:new AbortController().signal,fetcher:async(url,options)=>{seen.push([String(url),options]);return String(url).includes('/detail/')?response(String(url),'',0,'opaqueredirect'):response(String(url),'canonical');}});
 const out=await req('/index.php/index/problem/detail/pno/144/eno/9.html',{label:'题面检查 #144',problem:true});
 assert.equal(out.text,'canonical');assert.equal(out.url,origin+'/index.php/index/problem/detailtk/pno/144/eno/9.html');
 assert.equal(seen.length,2);assert.ok(seen.every(([,o])=>o.redirect==='manual'));
 await assert.rejects(req('https://other.invalid/problem',{problem:true}),/地址/);
});
test('unknown redirects and wrong readback URLs fail closed without following',async()=>{
 let calls=0;const make=fetcher=>H.createRequester({origin,signal:new AbortController().signal,fetcher});
 const req=make(async url=>{calls++;return response(String(url),'',0,'opaqueredirect');});
 await assert.rejects(req('/index.php/index/problem/detail/pno/144.html',{label:'题面检查 #144',problem:true}),/#144.*跳转/);assert.equal(calls,2);
 const bad=make(async()=>response(origin+'/index.php/index/problem/detail/pno/999.html'));
 await assert.rejects(bad('/index.php/index/problem/detail/pno/144.html',{label:'题面检查 #144',problem:true}),/地址/);
});
test('transient read failures recover with bounded backoff; judge POST is never retried',async()=>{
 let calls=0;const delays=[],retries=[];
 const make=fetcher=>H.createRequester({origin,signal:new AbortController().signal,fetcher,pause:async ms=>delays.push(ms),onRetry:label=>retries.push(label)});
 const req=make(async url=>{calls++;if(calls===1)throw TypeError('Load failed');if(calls===2)return response(String(url),'',503);return response(String(url),'recovered');});
 assert.equal((await req('/index.php/index/submissions/index.html?score_min=0',{label:'查询近期提交'})).text,'recovered');assert.equal(calls,3);assert.deepEqual(delays,[1000,2000]);assert.equal(retries.length,2);
 calls=0;const post=make(async()=>{calls++;throw TypeError('Load failed');});
 await assert.rejects(post('/index.php/index/index/prob_submit.html',{data:{pid:'144',language:'cpp',code:'int main(){}'},judge:true,label:'发送 #144'}),/#144/);assert.equal(calls,1);
 calls=0;await assert.rejects(post('/unexpected',{data:{x:'y'},label:'未知操作'}));assert.equal(calls,1);
});
test('source/template read POST retries safely, body failure and timeout are bounded, Stop cancels retries',async()=>{
 for(const path of ['/index.php/index/submissions/getcode.html','/index.php/index/problem/gettkcode.html']){
  let calls=0;const req=H.createRequester({origin,signal:new AbortController().signal,pause:async()=>{},fetcher:async url=>{calls++;return calls<3?{...response(String(url)),text:async()=>{throw TypeError('Load failed');}}:response(String(url),'source');}});
  assert.equal((await req(path,{data:{pno:'144'},label:'只读检查 #144'})).text,'source');assert.equal(calls,3);
 }
 const controller=new AbortController();let calls=0;
 const req=H.createRequester({origin,signal:controller.signal,fetcher:async()=>{calls++;throw TypeError('Load failed');},pause:async()=>controller.abort()});
 await assert.rejects(req('/index.php/index/submissions/index.html?score_min=0',{label:'查询记录'}),e=>e.name==='AbortError');assert.equal(calls,1);
 calls=0;const timeout=H.createRequester({origin,signal:new AbortController().signal,timeoutMs:5,pause:async()=>{},fetcher:async(url,{signal})=>{calls++;return new Promise((_,reject)=>signal.addEventListener('abort',()=>reject(new DOMException('timeout','AbortError'))));}});
 await assert.rejects(timeout('/index.php/index/submissions/index.html?score_min=0',{label:'查询记录'}),/查询记录.*超时/);assert.equal(calls,3);
});
test('a live single-slot template must exactly surround the approved full program after newline normalization',()=>{
 assert.equal(H.matchesTemplate('int main(){\r\n____qcodep____\r\n}\r\n','int main(){\nreturn 0;\n}\n'),true);
 assert.equal(H.matchesTemplate('a____qcodep____b','a\nanswer\nb'),true);
 for(const [template,code] of [['a____qcodep____b','changed\nanswer\nb'],['a____qcodep____b','ab'],['a____qcodep____b____qcodep____c','axbxc'],['a____qcodep____b','a____qcodep____b']])assert.equal(H.matchesTemplate(template,code),false);
});
test('resume validates only never-attempted problems so an old failed item cannot block the rest',async()=>{
 const seen=[];const f=fixture({attempted:()=>new Set([8]),preflightItem:async item=>{seen.push(item.problemNo);if(item.problemNo===8)throw Error('old inaccessible');}});
 await f.runner.start();assert.deepEqual(seen,[9]);assert.equal(f.runner.snapshot().phase,'DONE');assert.deepEqual(f.posts.map(p=>p.n),[9]);
});
test('an unavailable first or last item does not block verified items or mark it attempted',async()=>{
 for(const blocked of [8,9]){
  let probes=0;const f=fixture({baseline:async()=>{probes++;return 100;},preflightItem:async item=>{if(item.problemNo===blocked)throw H.itemUnavailable('permission denied');}});
  await f.runner.start();assert.equal(f.runner.snapshot().phase,'DONE');assert.deepEqual(f.remembered,[blocked===8?9:8]);
  assert.equal(f.runner.snapshot().rows.find(r=>r.problemNo===blocked).state,'BLOCKED');
  assert.equal(f.runner.snapshot().rows.find(r=>r.problemNo===blocked).reason,'permission denied');assert.ok(probes>=3);
 }
});
test('failed session recheck, unexpected preflight errors and Stop stay global failures',async()=>{
 let probes=0;const f=fixture({baseline:async()=>{if(++probes>1)throw Error('session unknown');return 100;},preflightItem:async()=>{throw H.itemUnavailable('item denied');}});
 await f.runner.start();assert.equal(f.runner.snapshot().phase,'STOPPED');assert.deepEqual(f.remembered,[]);assert.match(f.runner.snapshot().detail,/session unknown/);
 const g=fixture({preflightItem:async()=>{throw Error('unexpected bug');}});await g.runner.start();assert.equal(g.runner.snapshot().phase,'STOPPED');assert.deepEqual(g.remembered,[]);
 const h=fixture({preflightItem:async()=>{h.runner.stop();throw H.itemUnavailable('denied');}});await h.runner.start();assert.deepEqual(h.remembered,[]);assert.equal(h.runner.snapshot().phase,'STOPPED');
});
test('all blocked ends without attempts and a resumed batch rechecks only blocked items',async()=>{
 const remembered=new Set();let blocked=true;
 const make=()=>fixture({attempted:()=>remembered,remember:n=>remembered.add(n),preflightItem:async()=>{if(blocked)throw H.itemUnavailable('template unavailable');}});
 const f=make();await f.runner.start();assert.equal(f.runner.snapshot().phase,'DONE');assert.deepEqual(f.runner.snapshot().rows.map(r=>r.state),['BLOCKED','BLOCKED']);assert.equal(remembered.size,0);
 blocked=false;const g=make();await g.runner.start();assert.deepEqual(g.posts.map(p=>p.n),[8,9]);assert.ok(g.posts[1].at-g.posts[0].at>=3000);
 const h=make();await h.runner.start();assert.equal(h.posts.length,0);
});

test('older cached executors retain the array preflight contract',async()=>{
 const seen=[];const f=fixture({preflightItem:undefined,preflight:async items=>seen.push(...items.map(e=>e.problemNo))});
 await f.runner.start();assert.deepEqual(seen,[8,9]);assert.equal(f.posts.length,2);assert.equal(f.runner.snapshot().phase,'DONE');
});

test('Stop during item session recheck or lost lease after it prevents every POST',async()=>{
 let probes=0,release;const f=fixture({baseline:async()=>{if(++probes===2)await new Promise(r=>release=r);return 100;},preflightItem:async()=>{throw H.itemUnavailable('denied');}});
 const running=f.runner.start();await new Promise(r=>setImmediate(r));f.runner.stop();release();await running;
 assert.equal(f.runner.snapshot().phase,'STOPPED');assert.equal(f.posts.length,0);assert.equal(f.remembered.length,0);
 let guards=0;const g=fixture({guard:async()=>{if(++guards===3)throw Error('lost lease');},preflightItem:async()=>{throw H.itemUnavailable('denied');}});
 await g.runner.start();assert.equal(g.runner.snapshot().phase,'STOPPED');assert.match(g.runner.snapshot().detail,/lost lease/);assert.equal(g.posts.length,0);
});
