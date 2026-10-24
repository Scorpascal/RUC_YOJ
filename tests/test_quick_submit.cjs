// Run the shipped page and generated bookmarklet, not a second implementation.
// The small DOM doubles isolate browser/network APIs; live UI QA covers the DOM.
const {test} = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const {webcrypto,createHash}=require('node:crypto');
const B=require('../docs/assets/batch-submit.js');
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

async function page({url=contextUrl, clipboardFails=false, fallbackCopy=true, kind='STANDARD_CODE',badNativeHash=false}={}) {
  const elements = Object.fromEntries(['title','meta','source','notice','status','copy','submit','open','bookmarklet','code-heading','native-preview','native-source','native-hash'].map(id => [id,node()]));
  elements.source.firstElementChild = node();
  const body = node(), opened = [], copied = [], confirmations = [];
  const entry = {problemNo:1726,title:'LCA',language:'cpp17',problemUrl:url,
    submissionKind:kind,
    submitEndpoint:kind==='PARTIAL_CODE' ? 'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit_tk.html' : 'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit.html',codeUrl,
    codePath:decodeURIComponent(codeUrl.split('/main/')[1]),codeSha256:sha(source),onlineStatus:'Accepted',onlineSubmissionNo:'123'};
  if(kind==='PARTIAL_CODE') entry.nativeSubmission={kind:'VERIFIED_ASSEMBLED_CODE',
    codePath:'代码库/1726_complete.cpp',codeUrl:'https://raw.githubusercontent.com/Scorpascal/RUC_YOJ/main/'+encodeURI('代码库/1726_complete.cpp'),
    codeSha256:badNativeHash?'0'.repeat(64):sha(completeSource),submitEndpoint:'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit.html'};
  const catalog={entries:[{problemNo:1726,verified:true,publicationStatus:'PUBLIC_READY',onlineVerificationStatus:'ONLINE_ACCEPTED',
    language:'cpp17',codeUrl,codeSource:'PUBLIC_READY',quickSubmitMode:'verified',problemUrl:url,submissionNo:'123',
    onlineVerificationSubmissionNo:'123',...(entry.nativeSubmission?{nativeSubmission:entry.nativeSubmission}:{})}]};
  const window = {YOJBatch:B,confirm(message) { confirmations.push(message); return true; },
    open(...args) { opened.push(args); return {}; },setTimeout() {}};
  const context = vm.createContext({window,document:{body,getElementById:id=>elements[id],createElement:node,
    execCommand:()=>fallbackCopy},location:{search:'?pno=1726'},URL,URLSearchParams,crypto:webcrypto,
    navigator:{clipboard:{async writeText(value) { if (clipboardFails) throw Error('denied'); copied.push(value); }}},
    fetch:async url=>url==='data/quick-submit.json' ? {json:async()=>({schemaVersion:2,entries:[entry]})} :
      url==='data/catalog.json'?{json:async()=>catalog}:
      {ok:true,text:async()=>source,arrayBuffer:async()=>new TextEncoder().encode(url===entry.nativeSubmission?.codeUrl?completeSource:source).buffer}});
  vm.runInContext(pageScript, context);
  for(let i=0;i<40 && elements.copy.disabled && !elements.title.textContent.includes('失败');i++) await new Promise(r=>setTimeout(r,2));
  return {elements,body,opened,copied,confirmations,context};
}

async function fill(script, {url=contextUrl,pid='1726',hasForm=true,language=true,
  httpOk=true,body=source,selectLanguage=true,hasEditor=true,readbackMatches=true}={}) {
  let value = '', wrongEditorValue = '', fetchedUrl = '', fetches = 0, languageClicks = 0, submits = 0;
  const alerts = [], active = {value:false};
  const item = {click() {languageClicks++;active.value=selectLanguage;},
    classList:{contains:()=>active.value},getAttribute:name=>name==='data-value'?'cpp17':null};
  const form = {querySelector(selector) {
    if (selector.includes('pid')) return {value:pid};
    if (selector === "[name='code']") return {};
    if (selector.includes('data-value') || selector.includes('data-language')) return language ? item : null;
    if (selector.includes('.active')) return active.value ? item : null;
    return null;
  },submit() {submits++;}};
  const document = {querySelector(selector) {
    if (selector === '#submit_code' || selector === 'form#submit_code') return hasForm ? form : null;
    if (selector.includes('data-value') || selector.includes('data-language')) return language ? item : null;
    return null;
  },querySelectorAll:()=>[],getElementById:id=>id==='submit_code' && hasForm ? form : null};
  const editor = {setValue(next) {value=next;},getValue:()=>readbackMatches ? value : 'truncated'};
  const decoy = {setValue(next) {wrongEditorValue=next;},getValue:()=>wrongEditorValue};
  const context = vm.createContext({URL,location:new URL(url),document,window:{editor:hasEditor ? editor : undefined,
    monaco:{editor:{getEditors:()=>hasEditor ? [decoy] : []}}},
    alert:message=>alerts.push(message),
    fetch:async(url)=>{fetches++;fetchedUrl=new URL(url).href;return {ok:httpOk,status:httpOk?200:404,text:async()=>body};}});
  // Anchor href/bookmark URL normalization strips literal tabs and newlines.
  const normalized = new URL(script).href.slice('javascript:'.length);
  // javascript: navigation percent-decodes the URL before compiling its body.
  await vm.runInContext(decodeURIComponent(normalized), context);
  await tick();
  return {value,wrongEditorValue,fetchedUrl,fetches,languageClicks,submits,alerts};
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

for (const [label,opts] of [
  ['wrong problem',{url:plainUrl.replace('1726','1725'),pid:'1725'}],
  ['missing eno',{url:plainUrl}],
  ['wrong eno',{url:contextUrl.replace('2095','2096')}],
  ['lookalike origin',{url:contextUrl.replace('yoj.ruc.edu.cn','yoj.ruc.edu.cn.invalid.example')}],
  ['no accessible submit form',{hasForm:false}],
  ['mismatched hidden problem id',{pid:'1725'}],
  ['missing language',{language:false}],
]) test(`bookmarklet refuses ${label} before any fetch or edit`,async()=>{
  const p = await page();
  const result = await fill(p.elements.bookmarklet.href,opts);
  assert.equal(result.value,'');
  assert.equal(result.fetches,0);
  assert.equal(result.languageClicks,0);
  assert.equal(result.submits,0);
  assert.match(result.alerts[0]||'',/1726\/eno\/2095/);
});

test('bookmarklet fills the matching live form but never submits',async()=>{
  const p = await page();
  const result = await fill(p.elements.bookmarklet.href);
  assert.equal(result.value,source);
  assert.equal(result.wrongEditorValue,'');
  assert.equal(result.languageClicks,1);
  assert.equal(result.submits,0);
  assert.match(result.alerts[0],/已填入/);
});

test('bookmarklet preserves encoded Chinese and hash characters in the source path',async()=>{
  const p = await page();
  const result = await fill(p.elements.bookmarklet.href);
  assert.equal(result.fetchedUrl,'https://raw.githubusercontent.com/Scorpascal/RUC_YOJ/main/%E4%BB%A3%E7%A0%81%E5%BA%93/1726_%23_LCA.cpp');
});

for (const [label,opts,diagnostic] of [['no supported editor',{hasEditor:false},/找不到可安全填充的 YOJ 编辑器/],
  ['incomplete editor readback',{readbackMatches:false},/编辑器内容未完整写入/]])
  test(`bookmarklet reports ${label} without submitting`,async()=>{
    const p = await page();
    const result = await fill(p.elements.bookmarklet.href,opts);
    assert.equal(result.submits,0);
    assert.match(result.alerts[0]||'',diagnostic);
  });

for (const [label,opts] of [['HTTP error',{httpOk:false}],['empty source',{body:''}],
  ['language did not activate',{selectLanguage:false}]]) test(`bookmarklet refuses ${label}`,async()=>{
  const p = await page();
  const result = await fill(p.elements.bookmarklet.href,opts);
  assert.equal(result.value,'');
  assert.equal(result.submits,0);
  assert.match(result.alerts[0]||'',/失败/);
});
