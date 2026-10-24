// Optional rendered regression: node tests/browser/submit_theme.cjs
// Requires an existing Playwright installation (or PLAYWRIGHT_MODULE path).
// Network is restricted to local fixtures; this test never submits to YOJ.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const http = require('node:http');
const {chromium} = require(process.env.PLAYWRIGHT_MODULE || 'playwright');
const root = path.resolve(__dirname, '../..');
const docs = path.join(root, 'docs');

function luminance(color) {
  const channels = color.match(/[\d.]+/g).slice(0, 3).map(Number).map(n => {
    n /= 255;
    return n <= .04045 ? n / 12.92 : ((n + .055) / 1.055) ** 2.4;
  });
  return channels[0] * .2126 + channels[1] * .7152 + channels[2] * .0722;
}
function readable(fg, bg, label) {
  const a = luminance(fg), b = luminance(bg);
  assert.ok((Math.max(a, b) + .05) / (Math.min(a, b) + .05) >= 4.5, label);
}

(async () => {
  const server = http.createServer((req, res) => {
    const file = path.resolve(docs, '.' + new URL(req.url, 'http://localhost').pathname);
    if (!file.startsWith(docs + path.sep) || !fs.existsSync(file)) {
      res.writeHead(404).end(); return;
    }
    const type = {'.html': 'text/html', '.css': 'text/css', '.js': 'text/javascript', '.json': 'application/json'}[path.extname(file)];
    res.setHeader('Content-Type', (type || 'text/plain') + ';charset=utf-8');
    res.end(fs.readFileSync(file));
  });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  let browser;
  try {
    browser = await chromium.launch({headless: true});
    const context = await browser.newContext();
    const errors = [], judgeRequests = [];
    context.on('page', p => {
      p.on('pageerror', e => errors.push(e.message));
      p.on('console', m => {if (m.type() === 'error') errors.push(m.text());});
    });
    const base = `http://127.0.0.1:${server.address().port}`;
    await context.route('**/*', async route => {
      const url = route.request().url();
      const prefix = 'https://raw.githubusercontent.com/Scorpascal/RUC_YOJ/main/';
      if (url.startsWith(prefix)) {
        const source = path.resolve(root, decodeURIComponent(url.slice(prefix.length)));
        assert.ok(source.startsWith(root + path.sep));
        await route.fulfill({status: 200, body: fs.readFileSync(source),
          headers: {'Access-Control-Allow-Origin': '*'}});
      } else if (url.endsWith('/favicon.ico')) {
        await route.fulfill({status: 204});
      } else if (url.startsWith(base + '/')) {
        await route.continue();
      } else {
        judgeRequests.push(url);
        await route.abort();
      }
    });
    const batch = await context.newPage(), single = await context.newPage();
    const checks = [];
    for (const width of [1440, 390]) {
      for (const p of [batch, single]) await p.setViewportSize({width, height: 900});
      await batch.goto(base + '/yoj-batch-submit.html?pnos=1,1726,285,286');
      await single.goto(base + '/yoj-quick-submit.html?pno=1726');
      await batch.waitForFunction(() => !document.getElementById('start').disabled);
      await single.waitForFunction(() => !document.getElementById('submit').disabled);
      assert.match(await batch.title(), /批量提交/);
      assert.match(await single.title(), /YOJ/);
      for (const mode of ['dark', 'light']) {
        const palettes = [];
        for (const p of [batch, single]) {
          await p.emulateMedia({colorScheme: mode});
          const palette = await p.evaluate(() => {
            const card = getComputedStyle(document.querySelector('.card'));
            const notice = getComputedStyle(document.querySelector('.notice'));
            const muted = getComputedStyle(document.querySelector('.muted'));
            const link = getComputedStyle(document.querySelector('.card a'));
            const primary = getComputedStyle(document.querySelector('button'));
            return {card: card.backgroundColor, ink: card.color, muted: muted.color,
              notice: notice.backgroundColor, noticeInk: notice.color, link: link.color,
              primary: primary.backgroundColor, primaryInk: primary.color,
              bg: getComputedStyle(document.body).backgroundImage,
              overflow: document.documentElement.scrollWidth > innerWidth};
          });
          assert.equal(palette.overflow, false, 'page must fit mobile width');
          assert.ok(mode === 'dark' ? luminance(palette.card) < .05 : luminance(palette.card) > .9,
            `${mode}: card must follow system preference`);
          readable(palette.ink, palette.card, 'body text contrast');
          readable(palette.muted, palette.card, 'muted text contrast');
          readable(palette.link, palette.card, 'link contrast');
          readable(palette.noticeInk, palette.notice, 'warning contrast');
          readable(palette.primaryInk, palette.primary, 'primary button contrast');
          if (mode === 'dark') {
            const stops = palette.bg.match(/rgb\([^)]*\)/g) || [];
            assert.ok(stops.length >= 2 && stops.every(c => luminance(c) < .05),
              'dark background must not retain a light gradient stop');
          }
          palettes.push(palette);
        }
        for (const key of ['card', 'ink', 'muted', 'notice', 'noticeInk', 'link', 'primary', 'primaryInk', 'bg'])
          assert.equal(palettes[0][key], palettes[1][key], `${key}: both submit pages must share the palette`);
        await batch.locator('summary').first().click();
        assert.equal(await batch.locator('details').first().getAttribute('open'), '');
        assert.equal(await batch.evaluate(() => document.documentElement.scrollWidth > innerWidth), false);
        if (process.env.QA_SCREENSHOTS) {
          fs.mkdirSync(process.env.QA_SCREENSHOTS, {recursive: true});
          await batch.screenshot({path: path.join(process.env.QA_SCREENSHOTS, `batch-${mode}-${width}.png`), fullPage: true});
          await single.screenshot({path: path.join(process.env.QA_SCREENSHOTS, `single-${mode}-${width}.png`)});
        }
        await batch.locator('summary').first().click();
        checks.push(`${width}/${mode}`);
      }
      assert.equal(await batch.locator('#start').isEnabled(), true);
      assert.match(await batch.locator('#start').innerText(), /确认并开始/);
    }
    await batch.goto(base + '/yoj-batch-submit.html?pnos=0');
    await single.goto(base + '/yoj-quick-submit.html?pno=0');
    for (const p of [batch, single]) {
      await p.emulateMedia({colorScheme: 'dark'});
      const colors = await p.locator('#status').evaluate(el => [getComputedStyle(el).color,
        getComputedStyle(el.closest('.card')).backgroundColor]);
      readable(...colors, 'dark error text contrast');
    }
    assert.deepEqual(errors, []);
    assert.deepEqual(judgeRequests, []);
    console.log(JSON.stringify({checks, consoleErrors: 0, realJudgeRequests: 0,
      dynamicThemeSwitch: 'PASS', previewAndReady: 'PASS', darkErrors: 'PASS'}));
  } finally {
    await browser?.close();
    await new Promise(resolve => server.close(resolve));
  }
})().catch(e => {console.error(e); process.exitCode = 1;});
