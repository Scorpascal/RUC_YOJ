# Batch popup and session repair

**Scope:** Existing pure webpage batch submission, maximum ten problems and three-second spacing. User requested repair of first-item-only behavior and overlap between intentional batches. Keep original AC files, publication evidence/gates and nightly scheduling unchanged.

**Observed causes:** Production Safari site setting is Block and Notify; delayed independent response tabs can be blocked after the first user gesture. Old v1 session ledger stores problem numbers globally for the tab, so any overlap blocks the entire later batch. Previous Chromium fixtures did not exercise popup blocking.

**Approach:** Preserve independent native response pages. Before any judge POST, open and close an empty capability probe during the start gesture, then repeat after three seconds without another gesture. Failure blocks the batch with an explicit site-specific settings guide and no attempted-problem write. Do not bypass browser warnings, infer judge receipt or automatically retry. A probe cannot guarantee receipt or protect against permission changes during a run.

**Batch identity:** A random UUID in the review URL identifies one intentional batch. Versioned session records bind that identity to its ordered selection and attempted numbers. Refresh keeps the same identity and never resends attempted items; an explicit continuation sends only unattempted items. A URL-supplied identity with missing local history blocks all sends until an explicit New Batch action creates a fresh UUID. Homepage reselection or the explicit New Batch button creates a new identity, permitting overlaps after visible confirmation and a prior-attempt warning. Old v1 entries provide warnings only. No automatic expiry that silently retries an uncertain request.

**Alternatives:** Reusing response targets risks canceling pending requests; hidden HTTP frames risk mixed-content blocking. A YOJ-side helper could inspect responses but changes the approved pure-page workflow. Keep the existing transport and surface the browser capability requirement before any submission.

**Execution:** Inline implementation and one fresh independent review. No repeat live judge submissions for previously passed problems; Safari validation uses harmless loopback capability checks and local response fixtures. Recommended model for a separate implementation task: GPT-6.1-sol / high.

## Tasks

- [x] RED/GREEN: reproduce overlap failure in the actual page fixture; implement the batch-scoped ledger, explicit new/continue controls, and tests for selection/history binding, missing records, corruption and persistence failure.
- [x] RED/GREEN: delayed popup block prevents every judge POST and attempted-problem write; Stop during capability check cancels. Preserve independent forms, hash preflight and pacing.
- [x] Browser regression: all independent intercepted responses when allowed; Safari's measured blocked API result causes zero POSTs and resumes only after an explicit recheck. Cover same-ID refresh, copied URL with missing history, explicit overlap/new-batch behavior, Stop and mobile/dark/light layouts. Chromium empty-popup permissiveness is not claimed as Safari evidence.
- [x] Native Safari: Block and Notify measured first probe allowed and delayed probe blocked; production runner/transport stopped without any local fixture POST. With a test-only Allow exception, all three independent local fixture POSTs arrived. Restore test-only Block and Notify; zero real YOJ traffic.
- [x] Full regression: 62 Node tests, 280 Python tests, catalog/traffic/release consistency and rendered/intercepted browser checks pass. Independent review's missing-history and stale-status findings were reproduced RED and fixed GREEN; protected files remain unchanged.

**Release gate:** Scan every changed file and audit the exact staged tree before committing as Scorpascal. Use an ordinary push, confirm the matching Pages SHA and deployed file bytes, and retain the local delivery receipt until verified.
