const {test} = require('node:test');
const assert = require('node:assert/strict');
const {webcrypto, createHash} = require('node:crypto');
const fs = require('node:fs');
const vm = require('node:vm');
// A missing feature is an assertion failure, not a module-loader error.
const B = fs.existsSync('docs/assets/batch-submit.js') ? require('../docs/assets/batch-submit.js') : {};
const T = fs.existsSync('docs/assets/batch-transport.js') ? require('../docs/assets/batch-transport.js') : {};
const S = fs.existsSync('docs/assets/batch-session.js') ? require('../docs/assets/batch-session.js') : {};
const batchA='11111111-1111-4111-8111-111111111111', batchB='22222222-2222-4222-8222-222222222222';
const bytes = new TextEncoder().encode('int main() { return 0; }\n');
const sha = createHash('sha256').update(bytes).digest('hex');
const fullSource='int main() { return 7; }\n';
const fullBytes=new TextEncoder().encode(fullSource);
const fullSha=createHash('sha256').update(fullBytes).digest('hex');
function entry(n, extra={}) {
  const codePath = `代码库/${n}/code.cpp`;
  const result = {problemNo:n,title:`Problem ${n}`,language:'cpp17',onlineStatus:'Accepted',onlineSubmissionNo:String(1000000+n),codePath,
    problemUrl:`http://yoj.ruc.edu.cn/index.php/index/problem/detail/pno/${n}.html`,
    submitEndpoint:'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit.html',
    codeUrl:'https://raw.githubusercontent.com/Scorpascal/RUC_YOJ/main/'+encodeURI(codePath),codeSha256:sha,...extra};
  if (extra.submissionKind === 'PARTIAL_CODE' && !Object.hasOwn(extra,'nativeSubmission')) result.nativeSubmission={kind:'VERIFIED_ASSEMBLED_CODE',codePath:`代码库/${n}/complete.cpp`,codeUrl:'https://raw.githubusercontent.com/Scorpascal/RUC_YOJ/main/'+encodeURI(`代码库/${n}/complete.cpp`),codeSha256:fullSha,submitEndpoint:'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit.html'};
  return result;
}
function catalog(entries) {return {entries:entries.map(e=>({problemNo:e.problemNo,verified:true,
  publicationStatus:'PUBLIC_READY',onlineVerificationStatus:'ONLINE_ACCEPTED',language:e.language,
  codeUrl:e.codeUrl,problemUrl:e.problemUrl,codeSource:'PUBLIC_READY',quickSubmitMode:'verified',
  submissionNo:e.onlineSubmissionNo,onlineVerificationSubmissionNo:e.onlineSubmissionNo,...(e.nativeSubmission?{nativeSubmission:e.nativeSubmission}:{})}))};}
const manifest = entries => ({schemaVersion:2,entries,archivedEntries:[]});
const fetchCode = async url => ({ok:true,arrayBuffer:async()=>url?.endsWith('/complete.cpp')?fullBytes.buffer:bytes.buffer});
async function prepared(entries) {return B.prepareBatch(entries.map(e=>e.problemNo),manifest(entries),catalog(entries),fetchCode,webcrypto);}

test('selection rejects zero, eleven, duplicate and noninteger IDs, accepts ten',()=>{
  assert.equal(typeof B.parseNumbers,'function');
  for(const text of ['', '1,1','0','-1','1.1','1e2','1,,2',Array.from({length:11},(_,i)=>i+1).join(',')])
    assert.throws(()=>B.parseNumbers(text));
  assert.deepEqual(B.parseNumbers('1,2,3,4,5,6,7,8,9,10'),[1,2,3,4,5,6,7,8,9,10]);
});
test('preflight returns verified bytes and preserves eno context',async()=>{
  assert.equal(typeof B.prepareBatch,'function');
  const [item]=await prepared([entry(1726,{problemUrl:'http://yoj.ruc.edu.cn/index.php/index/problem/detail/pno/1726/eno/2095.html'})]);
  assert.equal(item.code,'int main() { return 0; }\n');
  assert.match(item.problemUrl,/\/1726\/eno\/2095\.html$/);
});
test('unpublished, archived-only, unknown or ambiguous records cannot enter a batch',async()=>{
  assert.equal(typeof B.prepareBatch,'function');
  const e=entry(1), good=catalog([e]);
  for(const [m,c] of [[manifest([]),good],[manifest([e,e]),good],
    [{...manifest([]),archivedEntries:[e]},good],
    [manifest([e]),{entries:[{...good.entries[0],publicationStatus:'RAW_CAPTURED'}]}],
    [manifest([e]),{entries:[{...good.entries[0],verified:false}]}],
    [manifest([e]),{entries:[{...good.entries[0],onlineVerificationStatus:'ONLINE_SKIPPED'}]}]])
    await assert.rejects(()=>B.prepareBatch([1],m,c,fetchCode,webcrypto));
});
test('bad source hash or a failing download blocks the whole preflight',async()=>{
  assert.equal(typeof B.prepareBatch,'function');
  await assert.rejects(()=>prepared([entry(1),entry(2,{codeSha256:'0'.repeat(64)})]));
  await assert.rejects(()=>B.prepareBatch([1],manifest([entry(1)]),catalog([entry(1)]),async()=>({ok:false,status:404}),webcrypto));
});
test('valid but inconsistent directory and quick records are blocked before downloads',async()=>{
  const e=entry(1), original=catalog([e]);
  for(const extra of [{codeUrl:entry(2).codeUrl,codePath:entry(2).codePath}, {language:'cpp'},
    {onlineSubmissionNo:'2000001'}, {problemUrl:'http://yoj.ruc.edu.cn/index.php/index/problem/detail/pno/1/eno/2095.html'}]) {
    let fetched=0;
    await assert.rejects(()=>B.prepareBatch([1],manifest([{...e,...extra}]),original,async()=>{fetched++;return fetchCode();},webcrypto));
    assert.equal(fetched,0);
  }
});
test('foreign code URLs, judge endpoints, mismatched pno and unknown forms are rejected',async()=>{
  assert.equal(typeof B.prepareBatch,'function');
  for(const extra of [{codeUrl:'https://example.invalid/code.cpp'},
    {submitEndpoint:'http://example.invalid/post'},{problemUrl:'http://yoj.ruc.edu.cn/index.php/index/problem/detail/pno/2.html'},
    {submissionKind:'MULTI_BLOCK'}, {onlineStatus:'Wrong Answer'}, {source:'ACCEPTED_ARCHIVE'},
    {codeUrl:'https://raw.githubusercontent.com/Other/repo/main/code.cpp'}])
    await assert.rejects(()=>prepared([entry(1,extra)]));
});
test('normal and single-slot browser routes send the independently verified program bytes',async()=>{
  assert.equal(typeof B.formFields,'function');
  const [normal,partial]=await prepared([entry(1726),entry(285,{submissionKind:'PARTIAL_CODE',
    submitEndpoint:'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit_tk.html'})]);
  assert.deepEqual(B.formFields(normal),{pid:'1726',language:'cpp17',code:'int main() { return 0; }\n'});
  assert.deepEqual(B.formFields(partial),{pid:'285',language:'cpp17',code:fullSource});
});

test('partial browser route fails closed if full-program evidence is missing or inconsistent',async()=>{
  const e=entry(285,{submissionKind:'PARTIAL_CODE',submitEndpoint:'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit_tk.html'});
  for(const nativeSubmission of [null,{...e.nativeSubmission,codeSha256:'0'.repeat(64)},
    {...e.nativeSubmission,codeUrl:'https://example.invalid/code.cpp'},
    {...e.nativeSubmission,submitEndpoint:'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit_tk.html'}]) {
    await assert.rejects(()=>B.prepareBatch([285],manifest([{...e,nativeSubmission}]),catalog([e]),fetchCode,webcrypto));
  }
});
test('matching malformed native projections still fail source and endpoint validation',async()=>{
  const e=entry(285,{submissionKind:'PARTIAL_CODE',submitEndpoint:'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit_tk.html'});
  for(const extra of [{codeSha256:'invalid'}, {codeSha256:'0'.repeat(64)},
    {codeUrl:'https://example.invalid/code.cpp'},
    {codePath:'代码库/../complete.cpp'},
    {submitEndpoint:'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit_tk.html'},
    {submitEndpoint:'http://example.invalid/index.php/index/index/prob_submit.html'}]) {
    const malformed={...e,nativeSubmission:{...e.nativeSubmission,...extra}};
    let downloads=0;
    await assert.rejects(()=>B.prepareBatch([285],manifest([malformed]),catalog([malformed]),async url=>{
      downloads++; return fetchCode(url);
    },webcrypto));
    assert.equal(downloads,extra.codeSha256==='0'.repeat(64)?2:0,
      'invalid metadata rejects before downloads; a well-formed wrong digest rejects at the full-byte hash gate');
  }
});
test('native metadata cannot override the validated problem or language with extra fields',async()=>{
  const e=entry(285,{submissionKind:'PARTIAL_CODE',submitEndpoint:'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit_tk.html'});
  const forged={...e,nativeSubmission:{...e.nativeSubmission,problemNo:999,language:'python3'}};
  let downloads=0;
  await assert.rejects(()=>B.prepareBatch([285],manifest([forged]),catalog([forged]),async url=>{
    downloads++; return fetchCode(url);
  },webcrypto));
  assert.equal(downloads,0);
});

function runner(items, overrides={}) {
  const sent=[], remembered=[], waits=[], states=[];
  const target={name:'batch-test',closed:false};
  let clock=0;
  const r=B.createRunner(items,{openTarget:()=>target,
    send(item,target){sent.push({pno:item.problemNo,name:target.name,at:clock});},
    remember(pno){remembered.push(pno);},
    async pause(ms){waits.push(ms);clock+=ms;},
    onChange(s){states.push(s);},...overrides});
  return {r,sent,remembered,waits,states,target};
}
test('failed popup preparation prevents every POST and attempted-problem write',async()=>{
  const t=runner(await prepared([entry(1),entry(2)]),{prepare:async()=>{throw Error('POPUP_BLOCKED');}});
  await t.r.start();
  assert.deepEqual(t.sent,[]);assert.deepEqual(t.remembered,[]);
  assert.equal(t.r.snapshot().phase,'BLOCKED');
});
test('Stop during popup preparation prevents all judge traffic',async()=>{
  let release;
  const t=runner(await prepared([entry(1),entry(2)]),{prepare:()=>new Promise(r=>{release=r;})});
  const active=t.r.start();t.r.stop();release?.();await active;
  assert.deepEqual(t.sent,[]);assert.deepEqual(t.remembered,[]);
  assert.equal(t.r.snapshot().phase,'STOPPED');
});
test('ten distinct requests are paced 3 seconds apart in one batch, never auto-restarted',async()=>{
  assert.equal(typeof B.createRunner,'function');
  const p=await prepared(Array.from({length:10},(_,i)=>entry(i+1)));
  const t=runner(p); await t.r.start(); await t.r.start();
  assert.deepEqual(t.sent.map(s=>s.pno),[1,2,3,4,5,6,7,8,9,10]);
  assert.deepEqual(t.sent.map(s=>s.at),[0,3000,6000,9000,12000,15000,18000,21000,24000,27000]);
  assert.equal(new Set(t.sent.map(s=>s.name)).size,1);
  assert.deepEqual(t.remembered,[1,2,3,4,5,6,7,8,9,10]);
  assert.equal(t.r.snapshot().phase,'DONE');
  assert.ok(t.r.snapshot().rows.every(row=>row.state==='REQUESTED'));
});
test('double-start during pacing cannot create a second queue',async()=>{
  assert.equal(typeof B.createRunner,'function');
  let release;
  const t=runner(await prepared([entry(1),entry(2)]),{pause:()=>new Promise(r=>{release=r;})});
  const active=t.r.start(); await t.r.start();
  assert.equal(t.sent.length,1); release(); await active;
  assert.deepEqual(t.sent.map(x=>x.pno),[1,2]);
});
test('stopping during pacing leaves remaining problems unsent',async()=>{
  assert.equal(typeof B.createRunner,'function');
  let release;
  const t=runner(await prepared([entry(1),entry(2)]),{pause:()=>new Promise(r=>{release=r;})});
  const active=t.r.start(); await new Promise(r=>setImmediate(r)); t.r.stop(); release(); await active;
  assert.deepEqual(t.sent.map(x=>x.pno),[1]);
  assert.equal(t.r.snapshot().phase,'STOPPED');
  await t.r.start(); assert.equal(t.sent.length,1);
});
test('pending native navigation keeps queue running and does not launch the next problem',async()=>{
  let response;
  const t=runner(await prepared([entry(1),entry(2)]),{send(item,name){
    t.sent.push({pno:item.problemNo,name});
    return new Promise(resolve=>{response=resolve;});
  }});
  const active=t.r.start();
  await new Promise(r=>setImmediate(r));
  assert.equal(t.r.snapshot().phase,'RUNNING');
  assert.deepEqual(t.sent.map(x=>x.pno),[1]);
  assert.equal(t.r.snapshot().rows[0].state,'UNCERTAIN');
  t.r.stop(); response(); await active;
  assert.equal(t.r.snapshot().phase,'STOPPED');
  assert.deepEqual(t.sent.map(x=>x.pno),[1]);
});
test('rejected native navigation stays uncertain and never advances or auto-retries',async()=>{
  const t=runner(await prepared([entry(1),entry(2)]),{send(){return Promise.reject(Error('navigation timeout'));}});
  await t.r.start(); await t.r.start();
  assert.equal(t.r.snapshot().phase,'STOPPED');
  assert.deepEqual(t.r.snapshot().rows.map(r=>r.state),['UNCERTAIN','WAITING']);
  assert.deepEqual(t.remembered,[1]);
});

function nativeDocument() {
  const sent=[], forms=[];
  const owner={body:{append(form){forms.push(form);}},createElement(tag){
    return {tag,children:[],append(child){this.children.push(child);},
      submit(){sent.push({form:this,target:this.target,action:this.action,
        fields:Object.fromEntries(this.children.map(c=>[c.name,c.value]))});}};
  }};
  return {owner,sent,forms};
}
test('native popup probe checks a delayed independent window before any form exists',async()=>{
  const f=nativeDocument();let opens=0,closed=0;
  const io=T.createTransport({document:f.owner,openWindow:()=>++opens===1?{closed:false,close(){closed++;}}:null,pause:async()=>{}});
  assert.equal(typeof io.prepare,'function');
  const target=io.openTarget();
  await assert.rejects(()=>io.prepare(target,new AbortController().signal),/弹窗/);
  assert.equal(closed,1);assert.equal(f.sent.length,0);assert.equal(f.forms.length,0);
});
test('native send cannot bypass a failed or missing popup preparation',async()=>{
  const f=nativeDocument();
  const io=T.createTransport({document:f.owner,openWindow:()=>null,pause:async()=>{}});
  await assert.rejects(()=>io.send({...entry(1),code:'A'},io.openTarget(),new AbortController().signal));
  assert.equal(f.sent.length,0);
});
test('native transport uses independent judge responses and closes its empty capability probes',async()=>{
  const f=nativeDocument(), probes=[];
  const io=T.createTransport({document:f.owner,pause:async()=>{},openWindow:(url,target)=>{
    assert.equal(url,'about:blank');assert.equal(target,'_blank');
    const p={closed:false,close(){this.closed=true;}};probes.push(p);return p;
  }});
  const target=io.openTarget(), signal=new AbortController().signal;
  await io.prepare(target,signal);
  await io.send({...entry(1726),code:'CODE'},target,signal);
  await io.send({...entry(285,{submissionKind:'PARTIAL_CODE'}),code:'FRAGMENT'},target,signal);
  assert.equal(f.sent.length,2);
  assert.notEqual(f.sent[0].form,f.sent[1].form);
  assert.ok(f.sent.every(p=>p.target==='_blank'));
  assert.ok(f.forms.every(p=>p.rel==='noopener' && p.hidden===true));
  assert.deepEqual(f.sent[0].fields,{pid:'1726',language:'cpp17',code:'CODE'});
  assert.deepEqual(f.sent[1].fields,{pno:'285',compiler:'cpp17',codes:'["FRAGMENT"]'});
  assert.equal(f.forms.length,2,'keep forms available while browser confirmation may still be pending');
  assert.equal(probes.length,2);assert.ok(probes.every(p=>p.closed));
});
test('aborted transport never starts a native POST',async()=>{
  const f=nativeDocument(), io=T.createTransport({document:f.owner,openWindow:()=>null}), c=new AbortController();
  c.abort();await assert.rejects(()=>io.send({...entry(1),code:'A'},io.openTarget(),c.signal),{name:'AbortError'});
  assert.equal(f.sent.length,0);
});
test('unavailable generic runner descriptor prevents form submissions',async()=>{
  assert.equal(typeof B.createRunner,'function');
  const blocked=runner(await prepared([entry(1)]),{openTarget:()=>null});
  await blocked.r.start(); assert.equal(blocked.sent.length,0);
  const t=runner(await prepared([entry(1),entry(2)]));
  t.target.closed=true; await t.r.start(); assert.equal(t.sent.length,0);
});
test('attempt is remembered before a possibly uncertain POST, which is never retried',async()=>{
  assert.equal(typeof B.createRunner,'function');
  const events=[];
  const t=runner(await prepared([entry(1),entry(2)]),{
    remember(n){events.push('remember:'+n);},send(item){events.push('send:'+item.problemNo);throw Error('unknown');}});
  await t.r.start(); await t.r.start();
  assert.deepEqual(events,['remember:1','send:1']);
  assert.equal(t.r.snapshot().rows[0].state,'UNCERTAIN');
  assert.equal(t.r.snapshot().rows[1].state,'WAITING');
});
test('storage failure stops before any POST',async()=>{
  assert.equal(typeof B.createRunner,'function');
  const t=runner(await prepared([entry(1)]),{remember(){throw Error('storage denied');}});
  await t.r.start(); assert.equal(t.sent.length,0);
});

test('invalidating a generic runner descriptor stops remaining requests',async()=>{
  const t=runner(await prepared([entry(1),entry(2)]),{async pause(){t.target.closed=true;}});
  await t.r.start(); assert.deepEqual(t.sent.map(x=>x.pno),[1]);
  assert.equal(t.r.snapshot().phase,'STOPPED');
});
test('all currently published entries pass preflight against their real local bytes',async()=>{
  const manifest=JSON.parse(fs.readFileSync('docs/data/quick-submit.json'));
  const cat=JSON.parse(fs.readFileSync('docs/data/catalog.json'));
  const load=async url=>({ok:true,arrayBuffer:async()=>{
    const file=decodeURIComponent(url.slice('https://raw.githubusercontent.com/Scorpascal/RUC_YOJ/main/'.length));
    const data=fs.readFileSync(file);return data.buffer.slice(data.byteOffset,data.byteOffset+data.byteLength);
  }});
  let checked=0;
  for(let i=0;i<manifest.entries.length;i+=10) {
    const ids=manifest.entries.slice(i,i+10).map(e=>e.problemNo);
    const prepared=await B.prepareBatch(ids,manifest,cat,load,webcrypto);
    checked+=prepared.length;
  }
  assert.equal(checked,manifest.entries.length);
});

test('batch ledger preserves attempts on reload while allowing intentional overlap in a new batch',()=>{
  assert.equal(typeof S.createLedger,'function');
  const data=new Map(), storage={getItem:k=>data.get(k)||null,setItem:(k,v)=>data.set(k,v)};
  const a=S.createLedger(storage,batchA,[1,2]);a.remember(1);
  assert.deepEqual([...S.createLedger(storage,batchA,[1,2]).attempted()],[1]);
  assert.throws(()=>a.remember(1));
  const b=S.createLedger(storage,batchB,[1,3]);
  assert.deepEqual([...b.attempted()],[]);assert.deepEqual(b.overlaps(),[1]);
  b.remember(1);assert.deepEqual([...b.attempted()],[1]);
  assert.throws(()=>S.createLedger(storage,batchA,[2,1]),/选择|题目/);
});
test('batch ledger validates selection and refuses corrupted or unwritable storage',()=>{
  assert.equal(typeof S.createLedger,'function');
  assert.throws(()=>S.createLedger({getItem:()=>'{',setItem(){}},batchA,[1]));
  assert.throws(()=>S.createLedger({getItem:()=>null,setItem(){throw Error('quota');}},batchA,[1]));
  const data=new Map(), storage={getItem:k=>data.get(k)||null,setItem:(k,v)=>data.set(k,v)};
  const a=S.createLedger(storage,batchA,[1]);assert.throws(()=>a.remember(2));
  storage.setItem=()=>{throw Error('quota');};assert.throws(()=>a.remember(1));
  assert.deepEqual([...a.attempted()],[]);
});
test('legacy problem-global ledger warns without blocking a new intentional batch',()=>{
  assert.equal(typeof S.createLedger,'function');
  const data=new Map([['yoj-batch-attempts-v1','[1,2]']]);
  const a=S.createLedger({getItem:k=>data.get(k)||null,setItem:(k,v)=>data.set(k,v)},batchA,[2,3]);
  assert.deepEqual([...a.attempted()],[]);assert.deepEqual(a.overlaps(),[2]);a.remember(2);
});
test('resuming an existing batch URL refuses missing storage or a missing batch row',()=>{
  const data=new Map(), storage={getItem:k=>data.get(k)||null,setItem:(k,v)=>data.set(k,v)};
  assert.throws(()=>S.createLedger(storage,batchA,[1],{resume:true}),/记录|批次/);
  S.createLedger(storage,batchB,[1]).remember(1);
  assert.throws(()=>S.createLedger(storage,batchA,[1],{resume:true}),/记录|批次/);
});

// Run the actual page; only DOM and external transport/timing are doubled.
async function batchPage({numbers='1726,285',batchId=batchA,resume=false,confirm=true,saved=new Map(),badHash=false,pause=async()=>{}}={}) {
  const elements=Object.fromEntries(['start','stop','status','count','queue','previews','new-batch','overlap','popup-help'].map(id=>[id,node()]));
  const sent=[],opens=[],confirmations=[],body=node();
  function node() {return {children:[],listeners:{},disabled:true,textContent:'',append(...items){this.children.push(...items);},
    addEventListener(event,fn){this.listeners[event]=fn;},remove(){},
    submit(){sent.push({form:this,action:this.action,target:this.target,fields:Object.fromEntries(this.children.map(c=>[c.name,c.value]))});}};}
  const entries=[entry(1726,{problemUrl:'http://yoj.ruc.edu.cn/index.php/index/problem/detail/pno/1726/eno/2095.html'}),
    entry(285,{submissionKind:'PARTIAL_CODE',submitEndpoint:'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit_tk.html',
      ...(badHash?{codeSha256:'0'.repeat(64)}:{})})];
  const query='?pnos='+numbers+(resume?'&batch='+batchId:'');
  const location={search:query,href:'https://example.invalid/yoj-batch-submit.html'+query};
  const ids=resume?[batchB]:[batchId,batchId===batchA?batchB:batchA];
  const ctx=vm.createContext({document:{body,getElementById:id=>elements[id],createElement:node},
    location,history:{replaceState(_,__,url){const u=new URL(url,location.href);location.href=u.href;location.search=u.search;}},URL,URLSearchParams,crypto:{subtle:webcrypto.subtle,randomUUID:()=>ids.shift()},
    sessionStorage:{getItem:k=>saved.get(k)||null,setItem:(k,v)=>saved.set(k,v)},
    localStorage:{getItem:k=>saved.get(k)||null,setItem:(k,v)=>saved.set(k,v)},navigator:{locks:{request:async(k,o,f)=>f({name:k})}},
    fetch:async url=>url==='data/quick-submit.json'?{ok:true,json:async()=>manifest(entries)}:
      url==='data/catalog.json'?{ok:true,json:async()=>catalog(entries)}:fetchCode(url),
    window:{YOJInPageEntry:require("../docs/assets/inpage-entry.js"),YOJBatchEntry:require("../docs/assets/batch-entry.js"),YOJBatchSession:S,YOJBatchTransport:{createTransport(options){return T.createTransport({...options,
      openWindow:()=>({closed:false,close(){}}),pause:async()=>{}});}},YOJBatch:{...B,createRunner(items,io){return B.createRunner(items,{...io,pause});}},
      open(url,name){opens.push({url,name});throw new Error('preopening is not allowed');},
      confirm(text){confirmations.push(text);return confirm;},addEventListener(){}}});
  vm.runInContext(fs.readFileSync('docs/assets/batch-page.js','utf8'),ctx);
  for(let i=0;i<40 && elements.status.textContent==='';i++) await new Promise(r=>setTimeout(r,2));
  return {elements,sent,opens,confirmations,saved};
}
test('intentional new batch can include problems attempted by an earlier batch',async()=>{
  const first=await batchPage({batchId:batchA});first.elements.start.listeners.click();
  await new Promise(r=>setImmediate(r));assert.equal(first.sent.length,2);
  const next=await batchPage({batchId:batchB,saved:first.saved});
  assert.equal(next.elements.start.disabled,false,'different batch must remain startable despite overlapping problems');
  assert.match(next.elements.overlap.textContent,/1726/);
  next.elements.start.listeners.click();await new Promise(r=>setImmediate(r));assert.equal(next.sent.length,2);
});
test('same batch identity cannot silently change its problem selection',async()=>{
  const first=await batchPage({batchId:batchA});
  const changed=await batchPage({batchId:batchA,resume:true,numbers:'1726',saved:first.saved});
  assert.equal(changed.elements.start.disabled,true);assert.match(changed.elements.status.textContent,/批次.*题目|选择/);
});
test('reload continuation sends only problems never attempted in this batch',async()=>{
  let release;
  const first=await batchPage({pause:()=>new Promise(r=>{release=r;})});
  first.elements.start.listeners.click();await new Promise(r=>setImmediate(r));
  first.elements.stop.listeners.click();release?.();await new Promise(r=>setImmediate(r));
  assert.equal(first.sent.length,1);
  const resumed=await batchPage({saved:first.saved,resume:true});
  assert.equal(resumed.elements.start.disabled,false);
  resumed.elements.start.listeners.click();await new Promise(r=>setImmediate(r));
  assert.deepEqual(resumed.sent.map(s=>s.fields.pid),['285']);
});
test('unknown batch URL holds sends until an explicit fresh batch is created',async()=>{
  const p=await batchPage({resume:true});
  assert.equal(p.elements.start.disabled,true);assert.equal(p.elements['new-batch'].disabled,false);
  assert.match(p.elements.status.textContent,/批次.*记录|记录.*批次/);assert.equal(p.sent.length,0);
  p.elements['new-batch'].listeners.click();
  assert.equal(p.elements.start.disabled,false);assert.equal(p.sent.length,0);
});
test('explicit new batch replaces the previous terminal status with a fresh ready message',async()=>{
  const p=await batchPage();p.elements.start.listeners.click();await new Promise(r=>setImmediate(r));
  assert.match(p.elements.status.textContent,/本批次已全部尝试/);
  p.elements['new-batch'].listeners.click();
  assert.match(p.elements.status.textContent,/尚未发送/);
  assert.doesNotMatch(p.elements.status.textContent,/已全部尝试/);
});
test('batch page never submits or asks for confirmation on load',async()=>{
  const p=await batchPage({confirm:false});
  assert.equal(p.elements.start.disabled,false);assert.equal(p.sent.length,0);
  assert.match(p.elements.status.textContent,/弹窗检测/);
  assert.equal(p.opens.length,0);assert.equal(p.confirmations.length,0);
});
test('one confirmed page click sends both native forms once and blocks refresh resends',async()=>{
  const p=await batchPage(); p.elements.start.listeners.click(); p.elements.start.listeners.click();
  await new Promise(r=>setImmediate(r));
  assert.equal(p.confirmations.length,0);assert.equal(p.opens.length,0);
  assert.deepEqual(p.sent.map(s=>s.fields),[
    {pid:'1726',language:'cpp17',code:'int main() { return 0; }\n'},
    {pid:'285',language:'cpp17',code:fullSource}]);
  assert.equal(new Set(p.sent.map(s=>s.form)).size,2);
  assert.ok(p.sent.every(s=>s.target==='_blank'));
  assert.match(p.elements.status.textContent,/不代表已受理或Accepted/);
  const refreshed=await batchPage({saved:p.saved,resume:true});
  assert.equal(refreshed.elements.start.disabled,true);
  assert.equal(refreshed.sent.length,0); assert.match(refreshed.elements.status.textContent,/不会自动重发/);
});
test('preflight failure in the last item never enables any page submission',async()=>{
  const p=await batchPage({badHash:true});
  assert.equal(p.elements.start.disabled,true);assert.equal(p.sent.length,0);assert.equal(p.opens.length,0);
  assert.match(p.elements.status.textContent,/哈希不匹配/);
});
