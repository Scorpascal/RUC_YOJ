// Run the shipped page and generated bookmarklet, not a second implementation.
// The small DOM doubles isolate browser/network APIs; live UI QA covers the DOM.
const {test} = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const {webcrypto,createHash}=require('node:crypto');
const B=require('../docs/assets/batch-submit.js'),H=require('../docs/assets/inpage-batch-core.js'),S=require('../docs/assets/batch-session.js'),E=require('../docs/assets/batch-entry.js');
const I=fs.existsSync('docs/assets/inpage-entry.js')?require('../docs/assets/inpage-entry.js'):{};
const completeSource='// verified assembled program\nint main() { return 0; }\n';
const sha=value=>createHash('sha256').update(value).digest('hex');

const html = fs.readFileSync('docs/yoj-quick-submit.html', 'utf8');
const pageScript = html.match(/<script>([\s\S]*?)<\/script>/)[1];
const plainUrl = 'http://yoj.ruc.edu.cn/index.php/index/problem/detail/pno/1726.html';
const contextUrl = 'http://yoj.ruc.edu.cn/index.php/index/problem/detail/pno/1726/eno/2095.html';
const source = 'int main() { return 0; }\n';
const codeUrl = 'https://raw.githubusercontent.com/Scorpascal/RUC_YOJ/main/%E4%BB%A3%E7%A0%81%E5%BA%93/1726_%23_LCA.cpp';
const tick = () => new Promise(resolve => setImmediate(resolve));

function node() {
  return {textContent:'', style:{}, children:[], disabled:true, href:'#', listeners:{},
    append(...items) { this.children.push(...items); this.lastElementChild = items.at(-1); },
    appendChild(item) { this.append(item); },
    set innerHTML(_) { this.lastElementChild = node(); },
    addEventListener(name, fn) { this.listeners[name] = fn; },
    select() {}, remove() {}, submit() { this.submitted = true; }};
}

async function page({url=contextUrl, clipboardFails=false, fallbackCopy=true, kind='STANDARD_CODE',badNativeHash=false,badHash=false,archived=false,pno=1726,stored,search='',popupBlocked=false,storageFails=false}={}) {
  const elements = Object.fromEntries(['title','meta','source','notice','status','copy','submit','open','bookmarklet','code-heading','native-preview','native-source','native-hash','inpage','new-single'].map(id => [id,node()]));
  elements.source.firstElementChild = node();
  const body = node(), opened = [], copied = [], confirmations = [];
  const entry = {problemNo:pno,title:'LCA',language:'cpp17',problemUrl:url,
    submissionKind:kind,
    submitEndpoint:kind==='PARTIAL_CODE' ? 'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit_tk.html' : 'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit.html',codeUrl,
    codePath:decodeURIComponent(codeUrl.split('/main/')[1]),codeSha256:badHash?'0'.repeat(64):sha(source),source:archived?'ACCEPTED_ARCHIVE':'PUBLIC_READY',onlineStatus:'Accepted',onlineSubmissionNo:'123'};
  if(kind==='PARTIAL_CODE') entry.nativeSubmission={kind:'VERIFIED_ASSEMBLED_CODE',
    codePath:'代码库/1726_complete.cpp',codeUrl:'https://raw.githubusercontent.com/Scorpascal/RUC_YOJ/main/'+encodeURI('代码库/1726_complete.cpp'),
    codeSha256:badNativeHash?'0'.repeat(64):sha(completeSource),submitEndpoint:'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit.html'};
  const catalog={entries:[{problemNo:pno,verified:true,publicationStatus:'PUBLIC_READY',onlineVerificationStatus:'ONLINE_ACCEPTED',
    language:'cpp17',codeUrl,codeSource:'PUBLIC_READY',quickSubmitMode:'verified',problemUrl:url,submissionNo:'123',
    onlineVerificationSubmissionNo:'123',...(entry.nativeSubmission?{nativeSubmission:entry.nativeSubmission}:{})}]};
  const data=stored||new Map(),storage={getItem:k=>data.get(k)??null,setItem:(k,v)=>{if(storageFails)throw Error('storage unavailable');data.set(k,v);}};
  const location=new URL('https://scorpascal.github.io/RUC_YOJ/yoj-quick-submit.html?pno='+pno+search);
  const window = {YOJBatch:B,YOJInPageCore:H,YOJBatchSession:S,YOJBatchEntry:E,YOJInPageEntry:I,confirm(message) { confirmations.push(message); return true; },
    open(...args) { if(popupBlocked)return null;const target={location:{href:args[0]},opener:{},close(){this.closed=true;}};opened.push(target);return target; },setTimeout() {}};
  const context = vm.createContext({window,document:{body,getElementById:id=>elements[id],createElement:node,
    execCommand:()=>fallbackCopy},location,history:{replaceState(a,b,url){location.href=url;}},sessionStorage:storage,localStorage:storage,URL,URLSearchParams,crypto:webcrypto,
    navigator:{locks:{async request(k,o,fn){return fn({name:k});}},clipboard:{async writeText(value) { if (clipboardFails) throw Error('denied'); copied.push(value); }}},
    fetch:async url=>url==='data/quick-submit.json' ? {ok:true,json:async()=>({schemaVersion:2,entries:archived?[]:[entry],archivedEntries:archived?[entry]:[]})} :
      url==='data/catalog.json'?{ok:true,json:async()=>catalog}:
      {ok:true,text:async()=>source,arrayBuffer:async()=>new TextEncoder().encode(url===entry.nativeSubmission?.codeUrl?completeSource:source).buffer}});
  vm.runInContext(pageScript, context);
  for(let i=0;i<40 && elements.copy.disabled && !elements.title.textContent.includes('失败');i++) await new Promise(r=>setTimeout(r,2));
  return {elements,body,opened,copied,confirmations,context,data,location};
}

test('eno shortcut preserves direct POST and offers the exact contextual problem link', async()=>{
  const p = await page();
  await p.elements.submit.listeners.click();
  await tick();
  const form = p.body.children.find(n=>n.method==='post');
  assert.ok(form?.submitted);
  assert.equal(form.action,'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit.html');
  assert.equal(p.elements.open.href,contextUrl);
  assert.deepEqual(Object.fromEntries(form.children.map(n=>[n.name,n.value])),{pid:'1726',language:'cpp17',code:source});
});

test('ordinary shortcut keeps explicit confirmation and the original POST fields',async()=>{
  const p = await page({url:plainUrl});
  await p.elements.submit.listeners.click();
  const form = p.body.children.find(n=>n.method==='post');
  assert.ok(form?.submitted);
  assert.deepEqual(Object.fromEntries(form.children.map(n=>[n.name,n.value])),{pid:'1726',language:'cpp17',code:source});
  assert.equal(p.confirmations.length,1);
  assert.equal(p.opened.length,0);
});

test('verified partial shortcut copies the fragment but submits its hash-checked complete program',async()=>{
  const p = await page({kind:'PARTIAL_CODE'});
  await p.elements.copy.listeners.click();
  assert.deepEqual(p.copied,[source]);
  assert.equal(p.elements['native-preview'].hidden,false);
  assert.equal(p.elements['native-source'].textContent,completeSource);
  await p.elements.submit.listeners.click();
  const form = p.body.children.find(n=>n.method==='post');
  assert.ok(form?.submitted);
  assert.equal(form.action,'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit.html');
  assert.deepEqual(Object.fromEntries(form.children.map(n=>[n.name,n.value])),
    {pid:'1726',language:'cpp17',code:completeSource});
});

test('partial native complete-code hash failure blocks submission',async()=>{
  const p=await page({kind:'PARTIAL_CODE',badNativeHash:true});
  assert.equal(p.elements.submit.disabled,true);
  assert.match(p.elements.status.textContent,/哈希不匹配/);
  assert.equal(p.body.children.filter(n=>n.method==='post').length,0);
});

test('copy failure is reported and does not navigate or submit',async()=>{
  const p = await page({clipboardFails:true,fallbackCopy:false});
  await p.elements.copy.listeners.click();
  await tick();
  assert.equal(p.opened.length,0);
  assert.equal(p.body.children.filter(n=>n.method==='post').length,0);
  assert.match(p.elements.status.textContent,/复制失败/);
});

test('repeated submit clicks cannot silently send the same code twice',async()=>{
  const p = await page();
  await p.elements.submit.listeners.click();
  await p.elements.submit.listeners.click();
  assert.equal(p.body.children.filter(n=>n.submitted).length,1);
  assert.equal(p.confirmations.length,1);
  assert.equal(p.elements.submit.disabled,true);
});

test('copy failure after a POST does not deny the earlier submission',async()=>{
  const p = await page({clipboardFails:true,fallbackCopy:false});
  await p.elements.submit.listeners.click();
  await p.elements.copy.listeners.click();
  assert.equal(p.body.children.filter(n=>n.submitted).length,1);
  assert.doesNotMatch(p.elements.status.textContent,/尚未发送/);
  assert.match(p.elements.status.textContent,/提交记录/);
});


test('one universal bookmark is reusable across different single problems',async()=>{
 const a=await page(),b=await page({pno:1715,url:plainUrl.replace('1726','1715')});
 assert.match(a.elements.bookmarklet.href,/^javascript:/);assert.equal(a.elements.bookmarklet.href,b.elements.bookmarklet.href);
 assert.equal(a.elements.inpage.disabled,false);assert.equal(b.elements.inpage.disabled,false);
});
test('single confirmation creates an exact one-item approval and opens YOJ without cross-site POST',async()=>{
 const p=await page();await p.elements.inpage.listeners.click();
 assert.equal(p.opened.length,1);const url=new URL(p.opened[0].location.href),id=url.searchParams.get('yoj_batch');
 assert.equal(url.origin,'http://yoj.ruc.edu.cn');assert.equal(url.pathname,'/index.php/index/submissions/index.html');
 const approved=H.loadApproved({getItem:k=>p.data.get(k)},id);
 assert.equal(approved.items.length,1);assert.equal(approved.items[0].problemNo,1726);assert.equal(approved.items[0].problemUrl,contextUrl);assert.equal(approved.items[0].codeSha256,sha(source));
 assert.equal(JSON.stringify(approved).includes(source),false);assert.equal(p.body.children.filter(n=>n.submitted).length,0);
 await p.elements.inpage.listeners.click();assert.equal(new URL(p.opened.at(-1).location.href).searchParams.get('yoj_batch'),id,'reopen keeps the same execution identity');
 await p.elements.submit.listeners.click();assert.equal(p.body.children.filter(n=>n.submitted).length,0,'inpage-bound task cannot switch to direct POST');
});
test('single explicit new submission gets a new identity; refresh preserves the previous mode',async()=>{
 const p=await page();await p.elements.inpage.listeners.click();const first=p.location.searchParams.get('batch');
 const resumed=await page({stored:p.data,search:'&batch='+first});assert.equal(resumed.elements.submit.disabled,true);assert.equal(resumed.elements.inpage.disabled,false);
 await resumed.elements['new-single'].listeners.click();assert.notEqual(resumed.location.searchParams.get('batch'),first);assert.equal(resumed.elements.submit.disabled,false);
});
test('direct single attempt prevents using the automatic mode for the same task',async()=>{
 const p=await page();await p.elements.submit.listeners.click();await p.elements.inpage.listeners.click();
 assert.equal(p.opened.length,0);assert.equal(p.body.children.filter(n=>n.submitted).length,1);
});
test('standard hash mismatch and historical archive cannot enter automatic submission',async()=>{
 const bad=await page({badHash:true});assert.equal(bad.elements.inpage.disabled,true);assert.equal(bad.elements.submit.disabled,true);assert.match(bad.elements.status.textContent,/哈希/);
 const old=await page({archived:true});assert.equal(old.elements.inpage.disabled,true);await old.elements.inpage.listeners.click();assert.equal(old.opened.length,0);
});
test('popup rejection and unavailable storage never create a submission',async()=>{
 const p=await page({popupBlocked:true});await p.elements.inpage.listeners.click();assert.equal(p.body.children.filter(n=>n.submitted).length,0);assert.match(p.elements.status.textContent,/阻止/);
 const q=await page({storageFails:true});assert.equal(q.elements.inpage.disabled,true);assert.equal(q.elements.submit.disabled,true);
});

test('lost single task history blocks implicit restart but allows an explicit new task',async()=>{
 const p=await page({search:'&batch=11111111-1111-4111-8111-111111111111'});
 assert.equal(p.elements.inpage.disabled,true);assert.equal(p.elements.submit.disabled,true);assert.equal(p.elements['new-single'].disabled,false);
 await p.elements['new-single'].listeners.click();assert.equal(p.elements.inpage.disabled,false);assert.notEqual(p.location.searchParams.get('batch'),'11111111-1111-4111-8111-111111111111');
});
