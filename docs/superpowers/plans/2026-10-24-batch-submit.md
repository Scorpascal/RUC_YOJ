# Batch Submit Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在现有静态主页增加最多 10 题、一次确认、3 秒间隔的批量提交。

**Architecture:** 独立 JS 模块负责清单校验、源码哈希、原生表单载荷和一次性串行队列。主页维护内存选题集合；独立批量页预加载源码、展示队列并操作一个复用的 YOJ 窗口。

**Tech Stack:** 现有 HTML/CSS/JavaScript，Web Crypto、sessionStorage、Node 内置测试；不增加运行依赖。

**Spec:** `docs/superpowers/specs/2026-10-24-batch-submit-design.md`

**Execution:** 本会话直接实施，最终一次独立审查。若另开 Codex 实现任务，推荐 GPT-6.1-sol / high。

## Global Constraints

- 最多 10 题，固定间隔 3 秒；结果仍到 YOJ 查看。
- 只使用通过既有 PUBLIC_READY 门禁的代码，不迁移登录态或凭据。
- 不改原有单题提交、AC 原件、脱敏、发布或定时时间。
- 不把浏览器发起动作当作原站受理或 Accepted；不自动重发。

## Review Focus

- 跨分页/筛选后选题应保留，满 10 题后仍能取消已选项。
- 构建清单/源码不同步、哈希不符或不安全 URL 必须在任何 POST 前阻断。
- 刷新、双击、停止、窗口关闭和发送状态不明不能导致自动重复提交。
- 旧的 PUBLIC_READY 题即使不在公开列表中仍可保留 eno 入口，但不得承诺账号具有访问权限。
- 表单目标复用及浏览器弹窗/混合内容限制必须如实说明，不能声称已获得评测结果。

### Task 1: Batch flow, integration and delivery

**Files:** Create `docs/assets/batch-submit.js`, `docs/assets/batch-page.js`, `docs/yoj-batch-submit.html`, `tests/test_batch_submit.cjs`; modify `docs/index.html`, `README.md`, `tools/build_initial.py`, `.github/workflows/pages.yml`.

**Interfaces:** `YOJBatch.parseNumbers(text)`, `prepareBatch(numbers, manifest, catalog, fetcher, crypto)`, `formFields(item)`, `createRunner(items, {openTarget, send, remember, pause, onChange})`; browser global plus CommonJS exports for tests. `prepareBatch` produces validated entries with byte-verified UTF-8 code; runner exposes `start()`, `stop()` and a read-only snapshot.

- [ ] Write failing Node tests for selection boundaries, manifest/catalog eligibility, raw SHA-256 validation, safe URLs, both payloads, eno and no partial preflight sends.
- [ ] Run `node --test tests/test_batch_submit.cjs`; observe missing feature failures.
- [ ] Implement minimal module and tests for one-shot runner, 3-second spacing, stop/window/uncertain POST/storage failures; run tests to green.
- [ ] Integrate homepage accessible checkboxes and review page, session attempt ledger and explicit confirmation; update README and its generator together, and include new tests in Pages CI.
- [ ] Run all Python tests, all Node tests, generated-data checks and consistency audit; expected zero failures.
- [ ] Browser QA: localhost homepage -> cross-page selection -> batch review -> locally intercepted forms; desktop and mobile, no real judge POSTs. Browser plugin absent: use Playwright.
- [ ] Independent read-only diff review, fix verified blockers, scan sensitive terms and exact staged tree.
- [ ] Commit atomically as Scorpascal, ordinary push main, verify remote identity and matching Pages deployment; preserve pending delivery checkpoint until confirmed.
