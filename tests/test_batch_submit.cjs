const {test} = require('node:test');
const assert = require('node:assert/strict');
const {webcrypto, createHash} = require('node:crypto');
const fs = require('node:fs');
const vm = require('node:vm');
// A missing feature is an assertion failure, not a module-loader error.
const B = fs.existsSync('docs/assets/batch-submit.js') ? require('../docs/assets/batch-submit.js') : {};
const bytes = new TextEncoder().encode('int main() { return 0; }\n');
const sha = createHash('sha256').update(bytes).digest('hex');
function entry(n, extra={}) {
  const codePath = `代码库/${n}/code.cpp`;
  return {problemNo:n,title:`Problem ${n}`,language:'cpp17',onlineStatus:'Accepted',onlineSubmissionNo:String(1000000+n),codePath,
    problemUrl:`http://yoj.ruc.edu.cn/index.php/index/problem/detail/pno/${n}.html`,
    submitEndpoint:'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit.html',
    codeUrl:'https://raw.githubusercontent.com/Scorpascal/RUC_YOJ/main/'+encodeURI(codePath),codeSha256:sha,...extra};
}
function catalog(entries) {return {entries:entries.map(e=>({problemNo:e.problemNo,verified:true,
  publicationStatus:'PUBLIC_READY',onlineVerificationStatus:'ONLINE_ACCEPTED',language:e.language,
  codeUrl:e.codeUrl,problemUrl:e.problemUrl,codeSource:'PUBLIC_READY',quickSubmitMode:'verified',
  submissionNo:e.onlineSubmissionNo,onlineVerificationSubmissionNo:e.onlineSubmissionNo}))};}
const manifest = entries => ({schemaVersion:2,entries,archivedEntries:[]});
const fetchCode = async () => ({ok:true,arrayBuffer:async()=>bytes.buffer});
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
test('normal and single-slot forms use the exact original fields',async()=>{
  assert.equal(typeof B.formFields,'function');
  const [normal,partial]=await prepared([entry(1726),entry(285,{submissionKind:'PARTIAL_CODE',
    submitEndpoint:'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit_tk.html'})]);
  assert.deepEqual(B.formFields(normal),{pid:'1726',language:'cpp17',code:'int main() { return 0; }\n'});
  assert.deepEqual(B.formFields(partial),{pno:'285',compiler:'cpp17',codes:'["int main() { return 0; }\\n"]'});
});

function runner(items, overrides={}) {
  const sent=[], remembered=[], waits=[], states=[];
  const target={name:'batch-test',closed:false};
  let clock=0;
  const r=B.createRunner(items,{openTarget:()=>target,
    send(item,name){sent.push({pno:item.problemNo,name,at:clock});},
    remember(pno){remembered.push(pno);},
    async pause(ms){waits.push(ms);clock+=ms;},
    onChange(s){states.push(s);},...overrides});
  return {r,sent,remembered,waits,states,target};
}
test('ten distinct requests are paced 3 seconds apart in one target, never auto-restarted',async()=>{
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
  const active=t.r.start(); t.r.stop(); release(); await active;
  assert.deepEqual(t.sent.map(x=>x.pno),[1]);
  assert.equal(t.r.snapshot().phase,'STOPPED');
  await t.r.start(); assert.equal(t.sent.length,1);
});
test('blocked or closed result window prevents further form submissions',async()=>{
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

test('closing the reused window after the first request stops remaining requests',async()=>{
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

// Run the actual page; only DOM and external transport/timing are doubled.
async function batchPage({numbers='1726,285',confirm=true,saved=new Map(),badHash=false}={}) {
  const elements=Object.fromEntries(['start','stop','status','count','queue','previews'].map(id=>[id,node()]));
  const sent=[],opens=[],confirmations=[],body=node();
  function node() {return {children:[],listeners:{},disabled:true,textContent:'',append(...items){this.children.push(...items);},
    addEventListener(event,fn){this.listeners[event]=fn;},remove(){},
    submit(){sent.push({action:this.action,target:this.target,fields:Object.fromEntries(this.children.map(c=>[c.name,c.value]))});}};}
  const entries=[entry(1726,{problemUrl:'http://yoj.ruc.edu.cn/index.php/index/problem/detail/pno/1726/eno/2095.html'}),
    entry(285,{submissionKind:'PARTIAL_CODE',submitEndpoint:'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit_tk.html',
      ...(badHash?{codeSha256:'0'.repeat(64)}:{})})];
  const ctx=vm.createContext({document:{body,getElementById:id=>elements[id],createElement:node},
    location:{search:'?pnos='+numbers},URLSearchParams,crypto:webcrypto,
    sessionStorage:{getItem:k=>saved.get(k)||null,setItem:(k,v)=>saved.set(k,v)},
    fetch:async url=>url==='data/quick-submit.json'?{ok:true,json:async()=>manifest(entries)}:
      url==='data/catalog.json'?{ok:true,json:async()=>catalog(entries)}:fetchCode(),
    window:{YOJBatch:{...B,createRunner(items,io){return B.createRunner(items,{...io,pause:async()=>{}});}},
      open(url,name){opens.push({url,name});return {closed:false};},
      confirm(text){confirmations.push(text);return confirm;},addEventListener(){}}});
  vm.runInContext(fs.readFileSync('docs/assets/batch-page.js','utf8'),ctx);
  for(let i=0;i<40 && elements.status.textContent==='';i++) await new Promise(r=>setTimeout(r,2));
  return {elements,sent,opens,confirmations,saved};
}
test('batch page never submits on load and cancellation opens no window',async()=>{
  const p=await batchPage({confirm:false});
  assert.equal(p.elements.start.disabled,false);assert.equal(p.sent.length,0);
  assert.match(p.elements.status.textContent,/至少需要 3 秒/);
  p.elements.start.listeners.click();
  assert.equal(p.sent.length,0);assert.equal(p.opens.length,0);assert.equal(p.confirmations.length,1);
});
test('one confirmed page click sends both native forms once and blocks refresh resends',async()=>{
  const p=await batchPage(); p.elements.start.listeners.click(); p.elements.start.listeners.click();
  await new Promise(r=>setImmediate(r));
  assert.equal(p.confirmations.length,1);assert.equal(p.opens.length,1);
  assert.deepEqual(p.sent.map(s=>s.fields),[
    {pid:'1726',language:'cpp17',code:'int main() { return 0; }\n'},
    {pno:'285',compiler:'cpp17',codes:'["int main() { return 0; }\\n"]'}]);
  assert.equal(new Set(p.sent.map(s=>s.target)).size,1);
  assert.match(p.elements.status.textContent,/不代表已受理或Accepted/);
  const refreshed=await batchPage({saved:p.saved});
  assert.equal(refreshed.elements.start.disabled,true);
  assert.equal(refreshed.sent.length,0); assert.match(refreshed.elements.status.textContent,/不会自动重发/);
});
test('preflight failure in the last item never enables any page submission',async()=>{
  const p=await batchPage({badHash:true});
  assert.equal(p.elements.start.disabled,true);assert.equal(p.sent.length,0);assert.equal(p.opens.length,0);
  assert.match(p.elements.status.textContent,/哈希不匹配/);
});
