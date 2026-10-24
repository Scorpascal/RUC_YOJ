// All YOJ and GitHub traffic intercepted; never sends a real judge request.
const {chromium}=require(process.env.PLAYWRIGHT_MODULE||'playwright');
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'../..'),site='https://scorpascal.github.io/RUC_YOJ/',origin='http://yoj.ruc.edu.cn';
(async()=>{
 const browser=await chromium.launch({headless:true});
 try{
  const context=await browser.newContext({viewport:{width:1400,height:1000}}),posts=[],errors=[],details=[];let logged=true,failHash=false,expireAfterPost=false,badTemplate=false,badTable=false,nonzeroEmpty=false;
  const auth='<a href="/index.php/index/login/logout.html">退出登录</a>';
  context.on('page',p=>p.on('pageerror',e=>errors.push(e.message)));
  await context.route('**/*',async route=>{
   const req=route.request(),url=req.url(),u=new URL(url),fields=Object.fromEntries(new URLSearchParams(req.postData()||''));
   if(url.startsWith(site)){
    const file=path.resolve(root,'docs',decodeURIComponent(u.pathname.slice('/RUC_YOJ/'.length)));
    assert.ok(file.startsWith(path.join(root,'docs')+path.sep));
    if(u.pathname.endsWith('/favicon.ico'))return route.fulfill({status:204});
    return route.fulfill({status:200,body:fs.readFileSync(file),contentType:({'.js':'text/javascript','.html':'text/html','.json':'application/json','.css':'text/css'})[path.extname(file)]||'text/plain'});
   }
   const raw='https://raw.githubusercontent.com/Scorpascal/RUC_YOJ/main/';
   if(url.startsWith(raw))return route.fulfill({status:200,body:failHash?'changed source':fs.readFileSync(path.join(root,decodeURIComponent(url.slice(raw.length)))),headers:{'Access-Control-Allow-Origin':'*'}});
   assert.equal(u.origin,origin,'Unexpected external destination');
   if(u.pathname.endsWith('/prob_submit.html')){
    assert.equal(req.method(),'POST');assert.equal(req.headers()['x-requested-with'],'XMLHttpRequest');if(fields.pid==='1726')assert.match(req.headers().referer,/pno\/1726\/eno\//);
    posts.push({fields,at:Date.now(),subno:900000+posts.length});if(expireAfterPost)logged=false;
    if(fields.pid==='8')return route.fulfill({status:302,headers:{location:origin+'/index.php/index/submissions/index.html'}});
    if(fields.pid==='285')return route.abort('connectionreset');
    return route.fulfill({json:{status:1}});
   }
   if(u.pathname.endsWith('/gettkcode.html'))return route.fulfill({json:badTemplate?{status:0}:{status:1,content:'a____qcodep____b'}});
   if(u.pathname.endsWith('/getcode.html'))return route.fulfill({json:{status:1,code:posts.find(p=>p.subno===Number(fields.subid)).fields.code}});
   if(u.pathname.includes('/problem/detail/pno/')){
    details.push(u.pathname);const n=Number(u.pathname.match(/pno\/(\d+)/)[1]);
    return route.fulfill({contentType:'text/html;charset=utf-8',body:auth+(n===285||n===286?`<h2>#${n}. Fixture</h2><select id="compiler_select"><option value="cpp">cpp</option></select><button id="btn-submit">提交</button>`:`<h2>#${n}. Fixture</h2><form id="submit_code"><input name="pid" value="${n}"><textarea name="code"></textarea><b data-value="cpp"></b><b data-value="c"></b><b data-value="cpp17"></b></form>`)});
   }
   if(u.pathname.endsWith('/favicon.ico'))return route.fulfill({status:204});
   if(badTable)return route.fulfill({contentType:'text/html;charset=utf-8',body:auth+'<p>temporary error</p>'});
   if(nonzeroEmpty)return route.fulfill({contentType:'text/html;charset=utf-8',body:auth+'<table><tr><th>编号</th><th>题目</th><th>状态</th></tr></table>10 条记录'});
   const rows=posts.slice().reverse().map(p=>`<tr><td><a href="/index.php/index/submissions/detail/subno/${p.subno}.html">#${p.subno}</a></td><td><a href="/index.php/index/problem/detail/pno/${p.fields.pid}.html">#${p.fields.pid}</a></td><td><span class="status">Accepted</span></td></tr>`).join('');
   return route.fulfill({contentType:'text/html;charset=utf-8',body:(logged?auth:'<a>登录</a>')+'<table><thead><tr><th>编号</th><th>题目</th><th>状态</th></tr></thead><tbody>'+rows+'</tbody></table>'+(posts.length?'':'0 条记录')});
  });
  const main=await context.newPage();await main.goto(site+'yoj-batch-submit.html?pnos=8,1726,285,286');
  await main.waitForFunction(()=>!document.getElementById('inpage').disabled);
  const bookmark=decodeURIComponent((await main.locator('#inpage-bookmark').getAttribute('href')).slice('javascript:'.length));
  const copied=await context.newPage();const sessionCopy=await main.evaluate(()=>sessionStorage.getItem('yoj-batch-sessions-v2'));await copied.addInitScript(value=>sessionStorage.setItem('yoj-batch-sessions-v2',value),sessionCopy);await copied.goto(main.url());await copied.waitForFunction(()=>!document.getElementById('start').disabled);
  const targetPromise=context.waitForEvent('page');await main.locator('#inpage').click();const target=await targetPromise;await target.waitForURL(/yoj_batch=/);await target.waitForLoadState();
  assert.ok(target.url().includes('yoj_batch='));assert.equal(await main.locator('#start').isDisabled(),true,'handoff binds mode');assert.equal(posts.length,0);
  await copied.locator('#start').click();await copied.waitForFunction(()=>document.getElementById('status').textContent.includes('另一执行方式'));assert.equal(posts.length,0,'copied pre-binding tab cannot switch mode');await copied.close();
  await target.evaluate(bookmark);
  await target.waitForFunction(()=>{const s=document.getElementById('yoj-inpage-panel')?.shadowRoot;return s&&s.querySelector('button').disabled;},{},{timeout:70000});
  const firstState=await target.evaluate(()=>document.getElementById('yoj-inpage-panel').shadowRoot.textContent);assert.match(firstState,/本批处理结束/);
  assert.deepEqual(posts.map(p=>Number(p.fields.pid)),[8,1726,285,286]);
  assert.ok(details.some(p=>p.includes('/pno/1726/eno/')));
  for(let i=1;i<posts.length;i++)assert.ok(posts[i].at-posts[i-1].at>=3000);
  assert.ok(posts.every(p=>Object.keys(p.fields).sort().join(',')==='code,language,pid'));
  for(const p of posts){const e=JSON.parse(fs.readFileSync(path.join(root,'docs/data/quick-submit.json'))).entries.find(e=>e.problemNo===Number(p.fields.pid));assert.equal(p.fields.code,fs.readFileSync(path.join(root,(e.nativeSubmission||e).codePath),'utf8'));}
  await target.evaluate(bookmark);assert.equal(posts.length,4,'duplicate bookmark does not create a queue');
  await target.reload();await target.evaluate(bookmark);
  await target.waitForFunction(()=>document.getElementById('yoj-inpage-panel')?.shadowRoot.textContent.includes('本批处理结束'));
  assert.equal(posts.length,4,'reload skips attempts across documents');
  await target.screenshot({path:'/private/tmp/yoj-inpage-fixture-desktop.png'});
  await target.setViewportSize({width:390,height:844});await target.screenshot({path:'/private/tmp/yoj-inpage-fixture-mobile.png'});
  failHash=true;await main.locator('#new-batch').click();const badPromise=context.waitForEvent('page');await main.locator('#inpage').click();const bad=await badPromise;await bad.waitForURL(/yoj_batch=/);await bad.waitForLoadState();await bad.evaluate(bookmark);
  await bad.waitForFunction(()=>document.getElementById('yoj-inpage-panel')?.shadowRoot.textContent.includes('核验失败'),{},{timeout:30000});
  assert.equal(posts.length,4,'hash failure sends no judge POST');
  failHash=false;badTemplate=true;await main.locator('#new-batch').click();
  const partialPromise=context.waitForEvent('page');await main.locator('#inpage').click();const partial=await partialPromise;await partial.waitForURL(/yoj_batch=/);await partial.waitForLoadState();await partial.evaluate(bookmark);
  await partial.waitForFunction(()=>document.getElementById('yoj-inpage-panel')?.shadowRoot.textContent.includes('当前填空模板不可用'));
  assert.equal(posts.length,4,'unavailable partial template blocks all POSTs');
  badTemplate=false;badTable=true;await main.locator('#new-batch').click();
  const malformedPromise=context.waitForEvent('page');await main.locator('#inpage').click();const malformed=await malformedPromise;await malformed.waitForURL(/yoj_batch=/);await malformed.waitForLoadState();await malformed.evaluate(bookmark);
  await malformed.waitForFunction(()=>document.getElementById('yoj-inpage-panel')?.shadowRoot.textContent.includes('无法确认原站提交列表结构'));
  assert.equal(posts.length,4,'malformed baseline is never treated as zero');
  badTable=false;nonzeroEmpty=true;await main.locator('#new-batch').click();
  const emptyPromise=context.waitForEvent('page');await main.locator('#inpage').click();const empty=await emptyPromise;await empty.waitForURL(/yoj_batch=/);await empty.waitForLoadState();await empty.evaluate(bookmark);
  await empty.waitForFunction(()=>document.getElementById('yoj-inpage-panel')?.shadowRoot.textContent.includes('无法确认原站空列表'));
  assert.equal(posts.length,4,'10 records is not a verified empty list');
  nonzeroEmpty=false;expireAfterPost=true;await main.locator('#new-batch').click();
  const expiringPromise=context.waitForEvent('page');await main.locator('#inpage').click();const expiring=await expiringPromise;await expiring.waitForURL(/yoj_batch=/);await expiring.waitForLoadState();await expiring.evaluate(bookmark);
  await expiring.waitForFunction(()=>document.getElementById('yoj-inpage-panel')?.shadowRoot.textContent.includes('登录已失效'),{},{timeout:30000});
  assert.equal(posts.length,5,'unknown response after session expiry stops the remaining batch');
  await expiring.evaluate(bookmark);assert.equal(posts.length,5,'duplicate bookmark never retries uncertain POST');
  logged=true;expireAfterPost=false;await main.locator('#new-batch').click();
  const racePromise=context.waitForEvent('page');await main.locator('#inpage').click();const raceA=await racePromise;await raceA.waitForURL(/yoj_batch=/);await raceA.waitForLoadState();
  const raceB=await context.newPage();await raceB.goto(raceA.url());
  await Promise.all([raceA.evaluate(bookmark),raceB.evaluate(bookmark)]);
  const terminal=p=>p.waitForFunction(()=>{const s=document.getElementById('yoj-inpage-panel')?.shadowRoot;return s&&s.querySelector('button').disabled;},{},{timeout:60000});
  await Promise.all([terminal(raceA),terminal(raceB)]);
  const outcomes=await Promise.all([raceA,raceB].map(p=>p.evaluate(()=>document.getElementById('yoj-inpage-panel').shadowRoot.textContent)));
  assert.equal(outcomes.filter(t=>t.includes('本批处理结束')).length,1);
  assert.equal(outcomes.filter(t=>t.includes('另一个YOJ批次正在运行')).length,1);
  assert.equal(posts.length,9,'atomic lease allows exactly one concurrent batch');
  assert.deepEqual(errors,[]);
  console.log(JSON.stringify({httpYOJWithSecureHashGate:'PASS',fourReceipts:'PASS',sourceRoundTrip:'PASS',contextPreserved:'PASS',threeSecondSpacing:'PASS',duplicateBookmarkAndReload:'PASS',hashFailure:'PASS',expiredSessionStops:'PASS',atomicLease:'PASS',templateFailure:'PASS',malformedBaseline:'PASS',realJudgeRequests:0,spacingMs:posts.slice(1,4).map((p,i)=>p.at-posts[i].at)}));
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
