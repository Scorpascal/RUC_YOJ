// Optional real-browser regression, using only local/intercepted fixtures.
// node tests/browser/submit_navigation.cjs (requires Playwright or PLAYWRIGHT_MODULE).
const {chromium} = require(process.env.PLAYWRIGHT_MODULE || 'playwright');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const http = require('node:http');
const root = path.resolve(__dirname, '../..'), docs = path.join(root, 'docs');
(async () => {
  const server = http.createServer((req, res) => {
    const file = path.resolve(docs, '.' + new URL(req.url, 'http://localhost').pathname);
    if (!file.startsWith(docs + path.sep) || !fs.existsSync(file)) {res.writeHead(404).end();return;}
    res.setHeader('Content-Type', ({'.html':'text/html','.js':'text/javascript','.css':'text/css','.json':'application/json'}[path.extname(file)] || 'text/plain') + ';charset=utf-8');
    let body = fs.readFileSync(file);
    // Negative control: reproduces the cancelled shared-window navigation
    // without editing production source or contacting the real judge.
    if (process.env.QA_FORCE_SHARED_TARGET && file.endsWith('/batch-transport.js'))
      body = body.toString().replace('form.target = "_blank"', 'form.target = "shared_regression_target"')
        .replace('form.rel = "noopener"', 'form.rel = ""');
    res.end(body);
  });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  const base = `http://127.0.0.1:${server.address().port}`;
  let browser;
  try {
    browser = await chromium.launch({headless:true});
    const context = await browser.newContext({viewport:{width:1440,height:1000}});
    const errors = [], requests = [], failed = [], finished = []; let confirmations = 0;
    context.on('requestfailed', request => {
      if (request.url().startsWith('http://yoj.ruc.edu.cn/')) failed.push(request.failure()?.errorText);
    });
    context.on('requestfinished', request => {
      if (request.url().startsWith('http://yoj.ruc.edu.cn/') && request.method() === 'POST')
        finished.push(Number(new URLSearchParams(request.postData()).get('pid') || new URLSearchParams(request.postData()).get('pno')));
    });
    context.on('page', page => page.on('pageerror', e => errors.push(e.message)));
    await context.route('**/*', async route => {
      const request = route.request(), url = request.url();
      const prefix = 'https://raw.githubusercontent.com/Scorpascal/RUC_YOJ/main/';
      if (url.startsWith(prefix)) {
        const file = path.resolve(root, decodeURIComponent(url.slice(prefix.length)));
        assert.ok(file.startsWith(root + path.sep));
        await route.fulfill({status:200,body:fs.readFileSync(file),headers:{'Access-Control-Allow-Origin':'*'}});
      } else if (url.endsWith('/favicon.ico')) await route.fulfill({status:204});
      else if (url.startsWith(base + '/')) await route.continue();
      else if (url.startsWith('http://yoj.ruc.edu.cn/') && request.method() === 'POST') {
        const row = {at:Date.now(), fields:Object.fromEntries(new URLSearchParams(request.postData()))};
        requests.push(row);
        await new Promise(resolve => setTimeout(resolve, requests.length === 1 ? 8000 : 20));
        const n = Number(row.fields.pid || row.fields.pno);
        await route.fulfill({status:200,contentType:'text/html',body:`<!doctype html><title>Intercepted fixture response</title><p>Fixture response ${n}; not a YOJ acceptance receipt.</p>`});
        row.respondedAt = Date.now();
      } else throw new Error('Unexpected external request: ' + url);
    });
    const page = await context.newPage();
    await page.addInitScript(() => {
      window.nativeSubmitTimes = [];
      const original = HTMLFormElement.prototype.submit;
      HTMLFormElement.prototype.submit = function () {
        window.nativeSubmitTimes.push(performance.now());
        return original.call(this);
      };
    });
    await page.goto(base + '/yoj-batch-submit.html?pnos=1,1726,285,286');
    await page.waitForFunction(() => !document.getElementById('start').disabled);
    page.on('dialog', d => {confirmations++;d.dismiss();});
    await page.locator('#start').click();
    await page.waitForFunction(() => !document.getElementById('stop').disabled);
    await page.waitForTimeout(3500);
    assert.deepEqual(failed, [], 'no pending response navigation may be cancelled');
    assert.equal(requests.length, 2, 'second request has an independent response while the first is pending');
    assert.equal(requests[0].respondedAt, undefined);
    assert.equal(await page.locator('#stop').isEnabled(), true);
    assert.match(await page.locator('#queue').innerText(), /已请求发起/);
    await page.waitForFunction(() => document.getElementById('status').textContent.startsWith('队列结束'), {}, {timeout:45000});
    assert.equal(confirmations, 0, 'the labelled start button is the only in-page confirmation');
    const deadline = Date.now() + 15000;
    while (requests.length < 4 || requests.some(r => !r.respondedAt) || context.pages().length < 5 || finished.length < 4) {
      assert.deepEqual(failed, [], 'no pending response navigation may be cancelled');
      assert.ok(Date.now() < deadline, 'all independent responses must finish');
      await page.waitForTimeout(100);
    }
    assert.equal(requests.length, 4);
    assert.equal(context.pages().length, 5, 'each native form opens an independent response page');
    assert.deepEqual(finished.sort((a,b)=>a-b), [1,285,286,1726]);
    assert.deepEqual(failed, []);
    const responseTexts = await Promise.all(context.pages().filter(p=>p!==page).map(async p=>{
      await p.waitForLoadState('load'); return p.locator('p').innerText();
    }));
    for (const n of [1,1726,285,286]) assert.ok(responseTexts.includes(`Fixture response ${n}; not a YOJ acceptance receipt.`));
    assert.deepEqual(requests.map(r => Number(r.fields.pid || r.fields.pno)), [1,1726,285,286]);
    const nativeTimes = await page.evaluate(() => window.nativeSubmitTimes);
    assert.equal(nativeTimes.length, 4);
    for (let i=1;i<nativeTimes.length;i++) assert.ok(nativeTimes[i] - nativeTimes[i-1] >= 3000, 'native POST calls must be at least 3000ms apart');
    assert.deepEqual(Object.keys(requests[2].fields).sort(), ['code','language','pid']);
    for (const row of requests.filter(r=>[285,286].includes(Number(r.fields.pid)))) {
      const manifest = JSON.parse(fs.readFileSync(path.join(docs,'data/quick-submit.json')));
      const entry = manifest.entries.find(e=>e.problemNo===Number(row.fields.pid));
      // Native HTML form serialization normalizes line endings to CRLF.
      // Assert that exact wire representation; source hashes remain raw-byte
      // checks in prepareBatch and no original file is rewritten.
      const source = fs.readFileSync(path.join(root,entry.nativeSubmission.codePath),'utf8');
      assert.equal(row.fields.code, source.replace(/\r\n|\r|\n/g,'\r\n'));
    }
    assert.deepEqual(Object.keys(requests[0].fields).sort(), ['code','language','pid']);
    assert.equal(requests[1].fields.language, 'cpp17');
    await page.reload();
    await page.waitForFunction(() => document.getElementById('status').textContent.includes('不会自动重发'));
    assert.equal(await page.locator('#start').isDisabled(), true);
    const stopped = await context.newPage();
    await stopped.goto(base + '/yoj-batch-submit.html?pnos=2,3');
    await stopped.waitForFunction(() => !document.getElementById('start').disabled);
    await stopped.locator('#start').click();
    await stopped.waitForFunction(() => !document.getElementById('stop').disabled);
    await stopped.locator('#stop').click();
    await stopped.waitForTimeout(3500);
    assert.equal(requests.length, 5, 'stop must cancel the unsent second problem');
    assert.match(await stopped.locator('#status').innerText(), /队列已停止/);
    assert.deepEqual(errors, []);
    console.log(JSON.stringify({independentPendingResponse:'PASS',stopEnabledWhilePending:'PASS',
      fourNativeForms:'PASS',independentPages:'PASS',oneClickConfirmation:'PASS',reloadGuard:'PASS',stop:'PASS',pageErrors:errors,
      requestSpacingMs:requests.slice(1,4).map((r,i)=>r.at-requests[i].at),
      nativeSubmitSpacingMs:nativeTimes.slice(1).map((t,i)=>t-nativeTimes[i]),realJudgeRequests:0}));
  } finally {await browser?.close();await new Promise(resolve => server.close(resolve));}
})().catch(e => {console.error(e);process.exitCode=1;});
