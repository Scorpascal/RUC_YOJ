# Safari Batch Repair Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans in this session. User approved the independent-response approach and requested minimum interaction.

**Goal:** 一次站内点击确认，最多十题按至少 3 秒间隔独立发起，不覆盖前一响应。

**Architecture:** 保留校验和一次性队列，用原生表单 target=_blank 独立响应替换共享预开窗口；不读取跨站结果，不伪装 Accepted。

**Tech Stack:** 现有静态 JS、HTML、Node tests；无新运行依赖。

**Spec:** `docs/superpowers/specs/2026-10-24-batch-submit-design.md`

## Global Constraints

- 不修改 AC、发布门禁、账号、定时或 3 秒间隔；不自动重发。
- 不能绕过浏览器安全警告；不能保证弹窗设置和安全确认仅需一次。
- 真实 Safari 核对 YOJ 新记录通过前不得推送；敏感测试证据只留忽略目录。

## Review Focus

- 第一响应延迟时后续不能取消或覆盖它。
- 一次点击、双击、刷新和停止不能造成重复提交。
- 普通和 eno 载荷保持；单填空的浏览器入口使用清单绑定且双哈希核验的完整组装程序，后台模板提交载荷不变。
- 弹窗被拦截也不能声称受理或自动重试。
- 前后台切换可以延长间隔，不能缩短到 3 秒以内。

### Task 1: Independent transport and one-click confirmation

**Files:** `docs/assets/batch-transport.js`, `docs/assets/batch-page.js`, `docs/assets/batch-submit.js`, `docs/yoj-batch-submit.html`, `tests/test_batch_submit.cjs`, `tests/browser/submit_navigation.cjs`, README and matching generator prose.

**Interfaces:** Preserve `createRunner(items, io)` and the validated form fields; `createTransport({document})` supplies a non-window batch descriptor and native `send(item, descriptor, signal)`.

**Ruling:** 原生专用填空 POST 实测失败，最小 Ajax 标记对照亦失败并已撤回；已有 PUBLIC_READY 完整组装程序通过原生普通入口实测 Accepted。为片段增加 `nativeSubmission` 投影并严格绑定 canonical 完整/组装哈希，批量及单题共享双哈希预检；不改原片段、后台模板门禁或定时。缺证据时阻断，不能以该兼容方案发布未经门禁的新代码。

- [x] Change tests first: no preopen, distinct native responses with delayed first response, one click without confirm dialog, stop/reload/no retry; observe RED.
- [x] Implement minimum native transport and truthful UI; keep original code hashes and fields unchanged. Run Node suite to GREEN.
- [x] Verify intercepted desktop/mobile/theme paths and full Python/Node regressions. Expected zero failed tests, zero real judge traffic in fixtures.
- [x] Safari: fresh baseline, small authorized batch including normal/1726/285/286, then new original-site IDs and evaluations; user handles any native security warnings. No blind repeat. Evidence retained locally; no repeat judge submissions during closeout.
- [x] Independent read-only review: no Critical/Important findings; extend matching-malformed-projection coverage and clarify the approved single-slot shortcut scope.

**Release procedure:** Audit the exact staged tree for privacy and consistency after the final regression. Commit/push as Scorpascal only after Safari acceptance; verify the matching Pages SHA and retain the release receipt locally.
