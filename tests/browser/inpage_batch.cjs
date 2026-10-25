// All YOJ and GitHub traffic intercepted; never sends a real judge request.
const {chromium}=require(process.env.PLAYWRIGHT_MODULE||'playwright');
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'../..'),site='https://scorpascal.github.io/RUC_YOJ/',origin='http://yoj.ruc.edu.cn';
(async()=>{
 const browser=await chromium.launch({headless:true});
 try{
  const context=await browser.newContext({viewport:{width:1400,height:1000}}),posts=[],errors=[],details=[],readFaults=new Set();let persistentReadFailure=false;const denied=new Set();let noProblemNavbar=false,expireOnDenied=false;let logged=true,failHash=false,expireAfterPost=false,badTemplate=false,badTable=false,nonzeroEmpty=false;
  context.setDefaultTimeout(20000);
  const auth='<a href="/index.php/index/login/logout.html">退出登录</a>';
  context.on('page',p=>p.on('pageerror',e=>errors.push(e.message)));
  await context.route('**/*',async route=>{
   const req=route.request(),url=req.url(),u=new URL(url),fields=Object.fromEntries(new URLSearchParams(req.postData()||''));
   if(url.startsWith(site)){
    const file=path.resolve(root,'docs',decodeURIComponent(u.pathname.slice('/RUC_YOJ/'.length)));
    assert.ok(file.startsWith(path.join(root,'docs')+path.sep));
    if(u.pathname.endsWith('/favicon.ico'))return route.fulfill({status:204});
    return route.fulfill({status:200,body:u.pathname.endsWith('/yoj-batch-submit.html')?fs.readFileSync(file,'utf8').replace('<script src="assets/inpage-entry.js"></script>',''):fs.readFileSync(file),contentType:({'.js':'text/javascript','.html':'text/html','.json':'application/json','.css':'text/css'})[path.extname(file)]||'text/plain'});
   }
   const raw='https://raw.githubusercontent.com/Scorpascal/RUC_YOJ/main/';
   if(url.startsWith(raw))return route.fulfill({status:200,body:failHash?'changed source':fs.readFileSync(path.join(root,decodeURIComponent(url.slice(raw.length)))),headers:{'Access-Control-Allow-Origin':'*'}});
   assert.equal(u.origin,origin,'Unexpected external destination');
   if(req.resourceType()==='fetch'&&!u.pathname.endsWith('/prob_submit.html')){
    const key=u.pathname.endsWith('/detailtk/pno/144.html')?'canonical':u.pathname.endsWith('/gettkcode.html')?'template':u.pathname.endsWith('/getcode.html')?'source':u.pathname.endsWith('/submissions/index.html')?'list':null;
    if(persistentReadFailure&&key==='canonical')return route.abort('connectionreset');
    if(key&&!readFaults.has(key)){readFaults.add(key);return key==='template'?route.fulfill({status:503}):route.abort('connectionreset');}
   }
   if(u.pathname.endsWith('/prob_submit.html')){
    assert.equal(req.method(),'POST');assert.equal(req.headers()['x-requested-with'],'XMLHttpRequest');if(fields.pid==='1726')assert.match(req.headers().referer,/pno\/1726\/eno\//);
    posts.push({fields,at:Date.now(),subno:900000+posts.length});if(expireAfterPost)logged=false;
    if(fields.pid==='8')return route.fulfill({status:302,headers:{location:origin+'/index.php/index/submissions/index.html'}});
    if(fields.pid==='285')return route.abort('connectionreset');
    return route.fulfill({json:{status:1}});
   }
   if(u.pathname.endsWith('/gettkcode.html')){const folders={144:'0144_子串替换',285:'0285_链表操作(加入节点)',286:'0286_链表操作(删除节点)'};return route.fulfill({json:badTemplate==='null'?null:badTemplate?{status:0}:{status:1,content:fs.readFileSync(path.join(root,'题解',folders[fields.pno],'assets','01_'+fields.pno+'.cpp'),'utf8')}});}
   if(u.pathname.endsWith('/getcode.html'))return route.fulfill({json:{status:1,code:posts.find(p=>p.subno===Number(fields.subid)).fields.code}});
   if(u.pathname==='/index.php/index/problem/detail/pno/144.html')return route.fulfill({status:302,headers:{location:origin+'/index.php/index/problem/detailtk/pno/144.html'}});
   if(/\/problem\/(detail|detailtk)\/pno\//.test(u.pathname)){
    details.push(u.pathname);const n=Number(u.pathname.match(/pno\/(\d+)/)[1]);
    if(denied.has(n)){if(expireOnDenied)logged=false;return route.fulfill({contentType:'text/html;charset=utf-8',body:'<h2>无权访问此课程题目</h2>'});}
    return route.fulfill({contentType:'text/html;charset=utf-8',body:(noProblemNavbar?'':auth)+(n===144||n===285||n===286?`<h2>#${n}. Fixture</h2><input name="pid" value="${n}"><select id="compiler_select"><option value="cpp">cpp</option></select><button id="btn-submit">提交</button>`:`<h2>#${n}. Fixture</h2><form id="submit_code"><input name="pid" value="${n}"><textarea name="code"></textarea><b data-value="cpp"></b><b data-value="c"></b><b data-value="cpp17"></b></form>`)});
   }
   if(u.pathname.endsWith('/favicon.ico'))return route.fulfill({status:204});
   if(badTable)return route.fulfill({contentType:'text/html;charset=utf-8',body:auth+'<p>temporary error</p>'});
   if(nonzeroEmpty)return route.fulfill({contentType:'text/html;charset=utf-8',body:auth+'<table><tr><th>编号</th><th>题目</th><th>状态</th></tr></table>10 条记录'});
   const rows=posts.slice().reverse().map(p=>`<tr><td><a href="/index.php/index/submissions/detail/subno/${p.subno}.html">#${p.subno}</a></td><td><a href="/index.php/index/problem/detail/pno/${p.fields.pid}.html">#${p.fields.pid}</a></td><td><span class="status">Accepted</span></td></tr>`).join('');
   return route.fulfill({contentType:'text/html;charset=utf-8',body:(logged?auth:'<input type="password">')+'<table><thead><tr><th>编号</th><th>题目</th><th>状态</th></tr></thead><tbody>'+rows+'</tbody></table>'+(posts.length?'':'0 条记录')});
  });
  const main=await context.newPage();await main.goto(site+'yoj-batch-submit.html?pnos=144,8,1726,285,286');
  await main.waitForFunction(()=>!document.getElementById('inpage').disabled);
  const bookmark=decodeURIComponent((await main.locator('#inpage-bookmark').getAttribute('href')).slice('javascript:'.length));
  const copied=await context.newPage();const sessionCopy=await main.evaluate(()=>sessionStorage.getItem('yoj-batch-sessions-v2'));await copied.addInitScript(value=>sessionStorage.setItem('yoj-batch-sessions-v2',value),sessionCopy);await copied.goto(main.url());await copied.waitForFunction(()=>!document.getElementById('start').disabled);
  const targetPromise=context.waitForEvent('page');await main.locator('#inpage').click();const target=await targetPromise;await target.waitForURL(/yoj_batch=/);await target.waitForLoadState();
  assert.ok(target.url().includes('yoj_batch='));assert.equal(await main.locator('#start').isDisabled(),true,'handoff binds mode');assert.equal(posts.length,0);
  await copied.locator('#start').click();await copied.waitForFunction(()=>document.getElementById('status').textContent.includes('另一执行方式'));assert.equal(posts.length,0,'copied pre-binding tab cannot switch mode');await copied.close();
  await target.evaluate(bookmark);
  await target.waitForFunction(()=>{const s=document.getElementById('yoj-inpage-panel')?.shadowRoot;return s&&s.querySelector('button').disabled;},{},{timeout:70000});
  const firstState=await target.evaluate(()=>document.getElementById('yoj-inpage-panel').shadowRoot.textContent);assert.match(firstState,/本批处理结束/);
  assert.deepEqual(posts.map(p=>Number(p.fields.pid)),[144,8,1726,285,286]);
  assert.ok(details.some(p=>p.includes('/pno/1726/eno/')));
  for(let i=1;i<posts.length;i++)assert.ok(posts[i].at-posts[i-1].at>=3000);
  assert.ok(posts.every(p=>Object.keys(p.fields).sort().join(',')==='code,language,pid'));
  for(const p of posts){const e=JSON.parse(fs.readFileSync(path.join(root,'docs/data/quick-submit.json'))).entries.find(e=>e.problemNo===Number(p.fields.pid));assert.equal(p.fields.code,fs.readFileSync(path.join(root,(e.nativeSubmission||e).codePath),'utf8'));}
  await target.evaluate(bookmark);assert.equal(posts.length,5,'duplicate bookmark does not create a queue');
  await target.reload();await target.evaluate(bookmark);
  await target.waitForFunction(()=>document.getElementById('yoj-inpage-panel')?.shadowRoot.textContent.includes('本批处理结束'));
  assert.equal(posts.length,5,'reload skips attempts across documents');
  await target.screenshot({path:'/private/tmp/yoj-inpage-fixture-desktop.png'});
  await target.setViewportSize({width:390,height:844});await target.screenshot({path:'/private/tmp/yoj-inpage-fixture-mobile.png'});
  failHash=true;await main.locator('#new-batch').click();const badPromise=context.waitForEvent('page');await main.locator('#inpage').click();const bad=await badPromise;await bad.waitForURL(/yoj_batch=/);await bad.waitForLoadState();await bad.evaluate(bookmark);
  await bad.waitForFunction(()=>document.getElementById('yoj-inpage-panel')?.shadowRoot.textContent.includes('核验失败'),{},{timeout:30000});
  assert.equal(posts.length,5,'hash failure sends no judge POST');
  failHash=false;badTemplate=true;await main.locator('#new-batch').click();
  const partialPromise=context.waitForEvent('page');await main.locator('#inpage').click();const partial=await partialPromise;await partial.waitForURL(/yoj_batch=/);await partial.waitForLoadState();await partial.evaluate(bookmark);
  await partial.waitForFunction(()=>document.getElementById('yoj-inpage-panel')?.shadowRoot.textContent.includes('本批处理结束'));
  assert.equal(posts.length,7,'unavailable templates skip only affected items');
  assert.deepEqual(posts.slice(5).map(p=>Number(p.fields.pid)),[8,1726]);
  assert.match(await partial.evaluate(()=>document.getElementById('yoj-inpage-panel').shadowRoot.textContent),/跳过 3 题/);
  badTemplate=false;badTable=true;await main.locator('#new-batch').click();
  const malformedPromise=context.waitForEvent('page');await main.locator('#inpage').click();const malformed=await malformedPromise;await malformed.waitForURL(/yoj_batch=/);await malformed.waitForLoadState();await malformed.evaluate(bookmark);
  await malformed.waitForFunction(()=>document.getElementById('yoj-inpage-panel')?.shadowRoot.textContent.includes('无法确认原站提交列表结构'));
  assert.equal(posts.length,7,'malformed baseline is never treated as zero');
  badTable=false;nonzeroEmpty=true;await main.locator('#new-batch').click();
  const emptyPromise=context.waitForEvent('page');await main.locator('#inpage').click();const empty=await emptyPromise;await empty.waitForURL(/yoj_batch=/);await empty.waitForLoadState();await empty.evaluate(bookmark);
  await empty.waitForFunction(()=>document.getElementById('yoj-inpage-panel')?.shadowRoot.textContent.includes('无法确认原站空列表'));
  assert.equal(posts.length,7,'10 records is not a verified empty list');
  nonzeroEmpty=false;expireAfterPost=true;await main.locator('#new-batch').click();
  const expiringPromise=context.waitForEvent('page');await main.locator('#inpage').click();const expiring=await expiringPromise;await expiring.waitForURL(/yoj_batch=/);await expiring.waitForLoadState();await expiring.evaluate(bookmark);
  await expiring.waitForFunction(()=>document.getElementById('yoj-inpage-panel')?.shadowRoot.textContent.includes('登录已失效'),{},{timeout:30000});
  assert.equal(posts.length,8,'unknown response after session expiry stops the remaining batch');
  await expiring.evaluate(bookmark);assert.equal(posts.length,8,'duplicate bookmark never retries uncertain POST');
  logged=true;expireAfterPost=false;await main.locator('#new-batch').click();
  const racePromise=context.waitForEvent('page');await main.locator('#inpage').click();const raceA=await racePromise;await raceA.waitForURL(/yoj_batch=/);await raceA.waitForLoadState();
  const raceB=await context.newPage();await raceB.goto(raceA.url());
  await Promise.all([raceA.evaluate(bookmark),raceB.evaluate(bookmark)]);
  const terminal=p=>p.waitForFunction(()=>{const s=document.getElementById('yoj-inpage-panel')?.shadowRoot;return s&&s.querySelector('button').disabled;},{},{timeout:60000});
  await Promise.all([terminal(raceA),terminal(raceB)]);
  const outcomes=await Promise.all([raceA,raceB].map(p=>p.evaluate(()=>document.getElementById('yoj-inpage-panel').shadowRoot.textContent)));
  assert.equal(outcomes.filter(t=>t.includes('本批处理结束')).length,1);
  assert.equal(outcomes.filter(t=>t.includes('另一个YOJ批次正在运行')).length,1);
  assert.equal(posts.length,13,'atomic lease allows exactly one concurrent batch');
  assert.deepEqual([...readFaults].sort(),['canonical','list','source','template']);
  persistentReadFailure=true;await main.locator('#new-batch').click();const stopPromise=context.waitForEvent('page');await main.locator('#inpage').click();const stopping=await stopPromise;await stopping.waitForURL(/yoj_batch=/);await stopping.waitForLoadState();await stopping.evaluate(bookmark);
  await stopping.waitForFunction(()=>document.getElementById('yoj-inpage-panel')?.shadowRoot.textContent.includes('安全重查'));
  await stopping.locator('#yoj-inpage-panel button').filter({hasText:'停止后续提交'}).click();
  await stopping.waitForTimeout(2200);assert.equal(posts.length,13,'Stop during read retry sends no judge POST');
  persistentReadFailure=false;await stopping.reload();await stopping.evaluate(bookmark);await terminal(stopping);
  assert.match(await stopping.evaluate(()=>document.getElementById('yoj-inpage-panel').shadowRoot.textContent),/本批处理结束/);assert.equal(posts.length,18,'same batch resumes after zero-attempt preflight failure');

  // An error page without the navbar is not a lost account session.
  async function fresh(){await main.locator('#new-batch').click();const next=context.waitForEvent('page');await main.locator('#inpage').click();const p=await next;await p.waitForURL(/yoj_batch=/);await p.waitForLoadState();await p.evaluate(bookmark);await terminal(p);return p;}
  badTemplate='null';let before=posts.length;const nullTemplate=await fresh();assert.match(await nullTemplate.evaluate(()=>document.getElementById('yoj-inpage-panel').shadowRoot.textContent),/本轮已受理 2 题，检查未通过跳过 3 题/);assert.deepEqual(posts.slice(before).map(p=>Number(p.fields.pid)),[8,1726]);badTemplate=false;
  denied.add(1726);before=posts.length;
  const deniedPage=await fresh();const deniedText=await deniedPage.evaluate(()=>document.getElementById('yoj-inpage-panel').shadowRoot.textContent);
  assert.match(deniedText,/本批处理结束：本轮已受理 4 题，检查未通过跳过 1 题/);assert.doesNotMatch(deniedText,/登录已失效/);
  assert.deepEqual(posts.slice(before).map(p=>Number(p.fields.pid)),[144,8,285,286]);
  for(let i=before+1;i<posts.length;i++)assert.ok(posts[i].at-posts[i-1].at>=3000,'blocked item preserves pacing');
  // Recovery keeps the same UUID; it must never resend the four received items.
  denied.clear();before=posts.length;await deniedPage.reload();await deniedPage.evaluate(bookmark);await terminal(deniedPage);
  assert.deepEqual(posts.slice(before).map(p=>Number(p.fields.pid)),[1726]);
  noProblemNavbar=true;before=posts.length;const navbarPage=await fresh();assert.match(await navbarPage.evaluate(()=>document.getElementById('yoj-inpage-panel').shadowRoot.textContent),/本轮已受理 5 题/);assert.equal(posts.length,before+5);noProblemNavbar=false;
  persistentReadFailure=true;before=posts.length;const netPage=await fresh();assert.match(await netPage.evaluate(()=>document.getElementById('yoj-inpage-panel').shadowRoot.textContent),/Load failed/);assert.deepEqual(posts.slice(before).map(p=>Number(p.fields.pid)),[8,1726,285,286]);persistentReadFailure=false;
  denied.add(1726);expireOnDenied=true;before=posts.length;const lost=await fresh();assert.match(await lost.evaluate(()=>document.getElementById('yoj-inpage-panel').shadowRoot.textContent),/登录已失效/);assert.equal(posts.length,before,'actual global expiry during preflight sends nothing');logged=true;denied.clear();expireOnDenied=false;

  // The exact same saved bookmark handles single, repeated new single, and batch.
  const single=await context.newPage();await single.goto(site+'yoj-quick-submit.html?pno=285');
  await single.waitForFunction(()=>!document.getElementById('inpage').disabled);
  assert.equal(decodeURIComponent((await single.locator('#bookmarklet').getAttribute('href')).slice(11)),bookmark);
  assert.match(await single.title(),/快捷提交/);assert.ok((await single.locator('#native-source').textContent()).length>100);
  await single.screenshot({path:'/private/tmp/yoj-universal-single-light.png',fullPage:true});
  await single.emulateMedia({colorScheme:'dark'});await single.setViewportSize({width:390,height:844});
  await single.screenshot({path:'/private/tmp/yoj-universal-single-dark-mobile.png',fullPage:true});
  async function fromSingle(){const next=context.waitForEvent('page');await single.locator('#inpage').click();const p=await next;await p.waitForURL(/yoj_batch=/);await p.waitForLoadState();await p.evaluate(bookmark);await terminal(p);return p;}
  before=posts.length;const one=await fromSingle();assert.deepEqual(posts.slice(before).map(p=>Number(p.fields.pid)),[285]);
  const full=JSON.parse(fs.readFileSync(path.join(root,'docs/data/quick-submit.json'))).entries.find(e=>e.problemNo===285).nativeSubmission;
  assert.equal(posts.at(-1).fields.code,fs.readFileSync(path.join(root,full.codePath),'utf8'));
  assert.match(await one.evaluate(()=>document.getElementById('yoj-inpage-panel').shadowRoot.textContent),/本轮已受理 1 题/);
  await one.evaluate(bookmark);await one.reload();await one.evaluate(bookmark);await terminal(one);assert.equal(posts.length,before+1);
  await single.reload();await single.waitForFunction(()=>!document.getElementById('inpage').disabled);assert.equal(await single.locator('#submit').isDisabled(),true);
  const reopened=await fromSingle();assert.equal(posts.length,before+1,'reopening a single task does not duplicate it');assert.equal(reopened.url(),one.url());
  await single.locator('#new-single').click();const second=await fromSingle();assert.notEqual(second.url(),one.url());assert.equal(posts.length,before+2,'explicit new task permits reuse of the same bookmark');
  await single.goto(site+'yoj-quick-submit.html?pno=144');await single.waitForFunction(()=>!document.getElementById('inpage').disabled);
  before=posts.length;await fromSingle();assert.deepEqual(posts.slice(before).map(p=>Number(p.fields.pid)),[144]);
  denied.add(1726);await single.goto(site+'yoj-quick-submit.html?pno=1726');await single.waitForFunction(()=>!document.getElementById('inpage').disabled);
  before=posts.length;const blockedSingle=await fromSingle();assert.equal(posts.length,before);assert.match(await blockedSingle.evaluate(()=>document.getElementById('yoj-inpage-panel').shadowRoot.textContent),/检查未通过跳过 1 题/);
  denied.clear();await blockedSingle.reload();await blockedSingle.evaluate(bookmark);await terminal(blockedSingle);assert.equal(posts.length,before+1);assert.equal(posts.at(-1).fields.pid,'1726');
  assert.deepEqual(errors,[]);
  console.log(JSON.stringify({httpYOJWithSecureHashGate:'PASS',fiveReceipts:'PASS',sourceRoundTrip:'PASS',contextPreserved:'PASS',threeSecondSpacing:'PASS',duplicateBookmarkAndReload:'PASS',hashFailure:'PASS',expiredSessionStops:'PASS',atomicLease:'PASS',templateIsolation:'PASS',contextPermissionIsolation:'PASS',missingNavbarSessionCheck:'PASS',blockedOnlyResume:'PASS',exhaustedItemReadIsolation:'PASS',globalPreflightExpiryStops:'PASS',malformedBaseline:'PASS',readNetworkRecovery:'PASS',stopDuringReadRetry:'PASS',sameBatchRecovery:'PASS',cachedHTMLDependencyRecovery:'PASS',universalSingleAndBatchBookmark:'PASS',singleExplicitRepeat:'PASS',singleReloadNoResend:'PASS',singleContextRecovery:'PASS',realJudgeRequests:0,spacingMs:posts.slice(1,4).map((p,i)=>p.at-posts[i].at)}));
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
