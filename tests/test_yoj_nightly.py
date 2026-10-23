"""Offline business-flow regressions; no judge, Keychain or remote Git calls."""

from __future__ import annotations

import fcntl
import json
import tempfile
import unittest
from contextlib import ExitStack
from datetime import datetime
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from tools import yoj_scheduler as scheduler, yoj_sync_status as status, yoj_capture


def at(value="2026-10-05T22:30:00+08:00"):
    return datetime.fromisoformat(value)


class NightlyTests(unittest.TestCase):
    def setUp(self):
        self.real_publish = scheduler.publish
        self.real_checkpoint = scheduler.publish_verified_checkpoint
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.state = self.root / ".yoj-sync"
        self.state.mkdir()
        self.snapshot = self.root / "public.json"
        self.snapshot.write_bytes(b"last valid snapshot")
        (self.root / "README.md").write_text("# Archive\n")
        self.now = at()
        self.events = []
        self.audit_failure = None
        self.discovery = {"scope": "account_visible_problem_numbers_only", "newProblemNumbers": []}
        self.capture = {
            "scope": "account_visible_problem_numbers_and_personal_accepted", "newProblemNumbers": [],
            "newTopicNumbers": [], "acTargetNumbers": [1717], "refreshedAcNumbers": [1717],
            "submissionScan": "FULL_TARGETED",
        }
        self.args = SimpleNamespace(allow_submit=True, publish=True, push=True, dry_run=False,
                                    visibility_watch=False, drift_audit=False, sentinel=False,
                                    request_interval=0, max_submissions=50)
        self.stack = ExitStack()
        self.addCleanup(self.stack.close)
        patches = {
            "ROOT": self.root, "STATE_DIR": self.state, "LOCK_PATH": self.state / "scheduler.lock",
            "PUBLIC_SNAPSHOT_PATH": self.snapshot,
        }
        for name, value in patches.items():
            self.stack.enter_context(patch.object(scheduler, name, value))
        for name, value in {
            "in_maintenance": False, "validate_worktree_before_run": True,
            "verify_pending_remote": True, "git_head_sha": "a" * 40,
            "child_environment": {}, "eligible_unpublished_numbers": [], "changed_paths": [],
            "snapshot_content_digest": "same", "run_visibility_audit": True,
            "load_visibility_report": {"hasVisibilityTransitions": False},
            "select_pending_personal_ac_targets": [], "select_raw_cleanup_backlog": [],
            "prepare_online_environment": {"YOJ_LOGIN_USER": "fixture"}, "publish_verified_checkpoint": 0, "publish": 0,
            "remember_generated_changes": None, "health_observe": None, "log": None,
        }.items():
            self.stack.enter_context(patch.object(scheduler, name, return_value=value))
        self.stack.enter_context(patch.object(scheduler, "now_local", side_effect=lambda: self.now))
        self.stack.enter_context(patch.object(scheduler, "run_command", side_effect=self.command))
        self.stack.enter_context(patch.object(scheduler, "load_capture_result", side_effect=lambda: self.discovery if not self.events or self.events[-1] == "discover" else self.capture))
        self.stack.enter_context(patch.object(scheduler.subprocess, "run", side_effect=self.git))
        self.stack.enter_context(patch.dict(scheduler.os.environ, {"YOJ_SYNC_ALLOW_PUSH": "1", "YOJ_SYNC_ENABLE_SUBMIT": "1"}))

    def git(self, command, **_kwargs):
        self.assertEqual(command[:3], ["git", "rev-parse", "origin/main"])
        return SimpleNamespace(returncode=0, stdout="a" * 40 + "\n", stderr="")

    def command(self, command, *_args, **_kwargs):
        name = Path(command[1]).name
        if name == "audit_online_availability.py":
            self.events.append("audit")
            if self.audit_failure:
                reason, state = self.audit_failure
                output = Path(command[command.index("--status-output") + 1])
                output.write_text(json.dumps({"schemaVersion": 1, "checkedAt": self.now.isoformat(),
                                               "sourceState": state, "reason": reason, "snapshotChanged": None}))
                return 1
        elif name == "yoj_capture.py":
            self.events.append("discover" if "--discover-only" in command else "capture")
            if "--account-visible" in command and "--discover-only" in command:
                (self.state / "account-visible-problems.json").write_text(json.dumps({
                    "schemaVersion": 1, "scope": "account_visible", "capturedAt": yoj_capture.utc_now(),
                    "pagesFetched": 1, "problemNumbers": [1717, 1725]}))
            if "--account-visible" not in command and "--discover-only" in command:
                self.discovery = {"scope": "public_problem_numbers_only", "newProblemNumbers": []}
        else:
            self.events.append(name)
        return 0

    def read(self):
        payload = status.read(self.state)
        self.assertIsNotNone(payload)
        return payload

    def successful_baseline(self):
        self.assertEqual(scheduler.run_once(self.args), 0)
        self.assertEqual(self.read()["syncState"], "SUCCEEDED")
        return self.read()["lastSuccessfulSyncAt"]

    def test_valid_unchanged_sync_updates_success_without_build_or_publish(self):
        self.successful_baseline()
        self.assertEqual(self.events, ["discover", "audit"])
        self.assertEqual(self.read()["sourceState"], "AVAILABLE")
        self.assertEqual(self.read()["lastSuccessfulSyncAt"], self.now.isoformat())
        self.assertEqual(self.snapshot.read_bytes(), b"last valid snapshot")
        scheduler.publish_verified_checkpoint.assert_not_called()
        self.assertIn("最近完整检查日期", (self.root / "README.md").read_text())

    def test_guard_invocation_binding_is_recorded_only_in_private_status(self):
        invocation = "a" * 32
        with patch.dict(scheduler.os.environ, {"YOJ_SYNC_INVOCATION_ID": invocation}):
            self.successful_baseline()
        self.assertEqual(self.read()["invocationId"], invocation)

    def test_unchanged_complete_run_writes_readme_check_date(self):
        readme = self.root / "README.md"
        readme.write_text("# Archive\n")
        with patch.object(scheduler, "publish", return_value=0):
            self.assertEqual(scheduler.run_once(self.args), 0)
        self.assertIn("最近完整检查日期（北京时间）：`2026-10-05`", readme.read_text())

    def test_no_change_still_requires_account_discovery(self):
        previous = "# Archive\n\n最近完整检查日期（北京时间）：`2026-10-04`\n"
        (self.root / "README.md").write_text(previous)
        with patch.object(scheduler, "prepare_online_environment", side_effect=RuntimeError("unavailable")):
            result = scheduler.run_once(self.args)
        self.assertNotEqual(result, 0)
        self.assertEqual(self.read()["syncState"], "INCOMPLETE")
        self.assertEqual(self.events, ["audit", "discover"])
        self.assertEqual((self.root / "README.md").read_text(), previous)

    def test_login_rejection_uses_public_fallback_without_reusing_account_snapshot(self):
        original = self.command
        def command(args, *rest):
            if "--account-visible" in args:
                self.events.append("rejected_login")
                return 3
            return original(args, *rest)
        original_result = scheduler.load_capture_result.side_effect
        with patch.object(scheduler, "run_command", side_effect=command), \
             patch.object(scheduler, "load_capture_result", side_effect=lambda: {
                 "scope": "account_login_unavailable"} if self.events[-1] == "rejected_login" else original_result()):
            self.assertEqual(scheduler.run_once(self.args), 3)
        self.assertEqual(self.events, ["rejected_login", "audit", "discover"])
        self.assertEqual(self.read()["reason"], "YOJ_AUTH_REQUIRED")
        self.assertNotIn("最近完整检查日期", (self.root / "README.md").read_text())

    def test_complete_check_date_is_stable_for_repeated_checks_same_day(self):
        self.successful_baseline()
        first = (self.root / "README.md").read_bytes()
        self.now = at("2026-10-05T23:15:00+08:00")
        self.successful_baseline()
        self.assertEqual((self.root / "README.md").read_bytes(), first)
        self.now = at("2026-10-06T22:30:00+08:00")
        self.successful_baseline()
        self.assertIn("`2026-10-06`", (self.root / "README.md").read_text())

    def test_authenticated_entry_failure_keeps_last_date_and_records_actual_source_reason(self):
        previous = '# Archive\n\n最近完整检查日期（北京时间）：`2026-10-21`\n'
        (self.root / 'README.md').write_text(previous)
        failure = {'scope': 'account_list_unavailable', 'authenticated': True,
                   'reason': 'YOJ_TRANSIENT_NETWORK_ERROR'}
        with patch.object(scheduler, 'run_command', return_value=1), \
             patch.object(scheduler, 'load_capture_result', return_value=failure):
            self.assertEqual(scheduler.run_once(self.args), 3)
        self.assertEqual((self.root / 'README.md').read_text(), previous)
        self.assertEqual(self.read()['sourceState'], 'TRANSIENT_ERROR')
        self.assertEqual(self.read()['reason'], 'YOJ_TRANSIENT_NETWORK_ERROR')

    def test_later_authenticated_no_change_run_advances_date_and_publishes(self):
        (self.root / 'README.md').write_text('# Archive\n\n最近完整检查日期（北京时间）：`2026-10-21`\n')
        self.now = at('2026-10-24T22:30:00+08:00')
        self.successful_baseline()
        self.assertIn('`2026-10-24`', (self.root / 'README.md').read_text())
        scheduler.publish.assert_called_once()

    def test_selected_success_keeps_existing_gate_order_and_publication(self):
        with patch.object(scheduler, "select_pending_personal_ac_targets", return_value=[1717]):
            self.successful_baseline()
        gates = ["build_initial.py", "normalize_cpp_headers.py", "port_cpp17_candidates.py",
                 "repair_known_candidates.py", "assemble_fill_in_candidates.py",
                 "sanitize_candidates.py", "validate_candidates.py",
                 "compile_candidates.py", "run_samples.py", "audit_forms.py", "audit_consistency.py"]
        self.assertEqual(self.events, ["discover", "audit", "capture", *gates, "online_verify.py"])
        scheduler.publish_verified_checkpoint.assert_called_once()

    def projection_fixture(self, *, audit_ok=True, pages_ok=True):
        """Run real selection/checkpoint/publisher; keep Git and child tools in memory.

        All files, status and locks belong to this test's temporary root.
        Removing selected topic propagation must lose the backlog statements;
        broadening the source allowlist must leak a raw sibling into the index.
        """
        statement = "题解/0285_backlog/0285_题目.md"
        metadata = "代码库/0285_backlog/0285_元数据.json"
        second_statement = "题解/0286_backlog/0286_题目.md"
        raw = "代码库/0285_backlog/0285_完整代码.cpp"
        verified = "代码库/0286_backlog/0286_完整代码.cpp"
        raw_sibling = "代码库/0286_backlog/0286_原始代码.cpp"
        excluded = [raw, raw_sibling, "题解/0285_backlog/raw.cpp", "题解/0285_backlog/private-log.json",
                    "题解/0286_backlog/unverified.cpp", "题解/0286_backlog/private-log.json", "题解/0999_unrelated.md",
                    "代码库/0999_unrelated/0999_元数据.json",
                    "staging/private-report.json", ".yoj-sync/private-report.json"]
        expected = ["README.md", "data/problems.json", statement, metadata,
                    second_statement, verified]
        dirty = expected + excluded
        for name in dirty:
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("{}\n" if name.endswith(".json") else "public fixture\n")
        (self.root / 'data/problems.json').write_text(json.dumps({'records': [
            {'problemNo': str(number), 'public': {'statement': name},
             'archive': {'metadata': metadata if number == 285 else ''},
             'statement': {'assets': [], 'imageAssets': []}}
            for number, name in [(285, statement), (286, second_statement)]]}))
        (self.root / "README.md").write_text(
            "# Archive\n\n最近完整检查日期（北京时间）：`2026-10-05`\n")
        ready = self.root / "data/public-ready.json"
        ready.write_text(json.dumps({"records": [{"problemNo": 286, "status": "PUBLIC_READY",
                                                  "completeCode": verified}]}))
        terms = self.root / "method/personal_terms.txt"
        terms.parent.mkdir()
        terms.write_text("fixture-personal-marker\n")
        self.stack.enter_context(patch.object(scheduler, "PUBLIC_READY_PATH", ready))
        pending = self.state / "pending.json"
        self.stack.enter_context(patch.object(scheduler, "PENDING_REMOTE_PATH", pending))
        self.stack.enter_context(patch.object(scheduler, "select_raw_cleanup_backlog", return_value=[285, 286]))
        self.stack.enter_context(patch.object(scheduler, "prepare_online_environment", return_value={
            "YOJ_LOGIN_USER": "runtime-account-value"}))
        self.stack.enter_context(patch.object(scheduler, "publish_verified_checkpoint", self.real_checkpoint))
        self.stack.enter_context(patch.object(scheduler, "publish", self.real_publish))
        self.stack.enter_context(patch.object(scheduler, "changed_paths", side_effect=lambda: list(dirty)))
        index, staged_batches, receipts = [], [], []
        head = ["a" * 40]
        self.stack.enter_context(patch.object(scheduler, "git_head_sha", side_effect=lambda: head[0]))
        self.stack.enter_context(patch.object(scheduler, "staged_paths", side_effect=lambda: list(index)))

        def git(command, **_kwargs):
            if command == ["git", "rev-parse", "origin/main"]:
                output = "a" * 40 + "\n"
            elif command == ["git", "branch", "--show-current"]:
                output = "main\n"
            elif command[:3] == ["git", "diff", "--check"]:
                output = ""
            elif command[:3] == ["git", "add", "--"]:
                index.extend(command[3:])
                staged_batches.append(list(index))
                output = ""
            elif command[:3] == ["git", "reset", "--"]:
                index.clear()
                output = ""
            elif command[:3] == ["git", "commit", "-m"]:
                receipts.append("commit")
                for name in index:
                    dirty.remove(name)
                index.clear()
                head[0] = "b" * 40
                output = "fixture commit\n"
            else:
                self.fail(f"Unexpected subprocess: {command!r}")
            return SimpleNamespace(returncode=0, stdout=output, stderr="")

        def audit():
            receipts.append("staged-audit")
            return audit_ok

        def push(sha):
            self.assertEqual(json.loads(pending.read_text())["state"], "push-pending")
            self.assertEqual(json.loads(pending.read_text())["commit"], sha)
            receipts.append(("push", sha))
            return True

        def pages(sha):
            self.assertEqual(json.loads(pending.read_text())["state"], "deployment-pending")
            self.assertEqual(json.loads(pending.read_text())["commit"], sha)
            receipts.append(("pages", sha))
            return pages_ok

        self.stack.enter_context(patch.object(scheduler.subprocess, "run", side_effect=git))
        self.stack.enter_context(patch.object(scheduler, "run_staged_consistency_audit", side_effect=audit))
        self.stack.enter_context(patch.object(scheduler, "push_pending_commit", side_effect=push))
        self.stack.enter_context(patch.object(scheduler, "verify_remote_workflow", side_effect=pages))
        return SimpleNamespace(statement=statement, expected=expected, excluded=excluded,
                               dirty=dirty, batches=staged_batches, receipts=receipts,
                               index=index, pending=pending)

    def test_backlog_statement_projection_reaches_real_publisher_without_raw_source(self):
        fixture = self.projection_fixture()
        self.successful_baseline()
        self.assertEqual(fixture.batches, [fixture.expected])
        self.assertEqual(fixture.dirty, fixture.excluded)
        self.assertEqual(fixture.receipts, ["staged-audit", "commit",
                                          ("push", "b" * 40), ("pages", "b" * 40)])
        self.assertFalse(fixture.pending.exists())
        self.assertEqual(self.events[-5:], ["publish_ready.py", "build_initial.py",
                                           "build_site_catalog.py", "build_site_catalog.py",
                                           "audit_consistency.py"])

    def test_backlog_projection_retains_personal_marker_scan(self):
        fixture = self.projection_fixture()
        (self.root / fixture.statement).write_text("fixture-personal-marker\n")
        self.assertEqual(scheduler.run_once(self.args), 7)
        self.assertEqual(fixture.batches, [])
        self.assertEqual(fixture.receipts, [])

    def test_backlog_projection_retains_runtime_secret_scan(self):
        fixture = self.projection_fixture()
        (self.root / fixture.statement).write_text("fixture-runtime-secret\n")
        with patch.object(scheduler, "prepare_online_environment", return_value={
                "YOJ_LOGIN_USER": "fixture", "YOJ_LOGIN_PASS": "fixture-runtime-secret"}):
            self.assertEqual(scheduler.run_once(self.args), 7)
        self.assertEqual(fixture.batches, [])
        self.assertEqual(fixture.receipts, [])

    def test_backlog_projection_cannot_commit_when_staged_tree_audit_fails(self):
        fixture = self.projection_fixture(audit_ok=False)
        self.assertEqual(scheduler.run_once(self.args), 5)
        self.assertEqual(fixture.batches, [fixture.expected])
        self.assertEqual(fixture.index, [])
        self.assertEqual(fixture.receipts, ["staged-audit"])
        self.assertFalse(fixture.pending.exists())

    def test_backlog_projection_keeps_exact_commit_pending_until_pages_success(self):
        fixture = self.projection_fixture(pages_ok=False)
        self.assertEqual(scheduler.run_once(self.args), 8)
        checkpoint = json.loads(fixture.pending.read_text())
        self.assertEqual(checkpoint["commit"], "b" * 40)
        self.assertEqual(checkpoint["state"], "deployment-pending")
        self.assertEqual(fixture.receipts[-2:], [("push", "b" * 40), ("pages", "b" * 40)])
        self.assertEqual(self.read()["syncState"], "INCOMPLETE")

    def test_assembly_failure_stops_before_sanitization_online_and_publication(self):
        original = self.command

        def command(args, *rest):
            result = original(args, *rest)
            if Path(args[1]).name == "assemble_fill_in_candidates.py":
                self.assertEqual(args[2:], ["--problem", "285", "--problem", "286"])
                return 1
            return result

        with patch.object(scheduler, "select_raw_cleanup_backlog", return_value=[285, 286]), \
             patch.object(scheduler, "run_command", side_effect=command):
            self.assertEqual(scheduler.run_once(self.args), 3)
        self.assertEqual(self.events[-2:], ["repair_known_candidates.py", "assemble_fill_in_candidates.py"])
        self.assertNotIn("sanitize_candidates.py", self.events)
        self.assertNotIn("online_verify.py", self.events)
        scheduler.publish_verified_checkpoint.assert_not_called()

    def test_context_only_refresh_still_runs_selected_gates_and_publication(self):
        self.capture['refreshedAcNumbers'] = []
        self.capture['contextUpdatedNumbers'] = [1717]
        with patch.object(scheduler, 'select_pending_personal_ac_targets', return_value=[1717]):
            self.successful_baseline()
        self.assertIn('sanitize_candidates.py', self.events)
        self.assertIn('online_verify.py', self.events)
        scheduler.publish_verified_checkpoint.assert_called_once()

    def test_tls_cooldown_preserves_data_and_local_checkpoint_then_recovers(self):
        last = self.successful_baseline()
        self.now = at("2026-10-05T22:31:00+08:00")
        self.audit_failure = ("YOJ_TLS_CERTIFICATE_ERROR", "BLOCKED")
        self.assertEqual(scheduler.run_once(self.args), 3)
        deadline = self.read()["retryAfter"]
        self.now = at("2026-10-05T22:45:00+08:00")
        self.events.clear()
        with patch.object(scheduler, "eligible_unpublished_numbers", return_value=[42]), \
             patch.object(scheduler, "publish_verified_checkpoint", side_effect=lambda *_a: self.events.append("release") or 0):
            self.assertEqual(scheduler.run_once(self.args), 3)
        self.assertEqual(self.events, ["release", "discover"])
        scheduler.verify_pending_remote.assert_called()
        self.assertEqual(self.read()["retryAfter"], deadline)
        self.assertEqual(self.read()["lastSuccessfulSyncAt"], last)
        self.assertEqual(self.snapshot.read_bytes(), b"last valid snapshot")
        self.now = at("2026-10-05T23:01:00+08:00")
        self.audit_failure = None
        self.events.clear()
        self.assertEqual(scheduler.run_once(self.args), 0)
        self.assertEqual(self.events, ["discover", "audit"])
        self.assertIsNone(self.read()["retryAfter"])
        self.assertEqual(self.read()["lastSuccessfulSyncAt"], self.now.isoformat())

    def test_blocking_categories_cool_down_but_transient_and_invalid_source_do_not(self):
        for reason, source in [("YOJ_CAMPUS_ACCESS_REQUIRED", "BLOCKED"), ("YOJ_AUTH_REQUIRED", "BLOCKED"),
                               ("YOJ_TRANSIENT_NETWORK_ERROR", "TRANSIENT_ERROR"), ("YOJ_INVALID_PUBLIC_INDEX", "ERROR")]:
            with self.subTest(reason=reason):
                (self.state / "sync-status.json").unlink(missing_ok=True)
                self.audit_failure = (reason, source)
                self.events.clear()
                self.assertEqual(scheduler.run_once(self.args), 3)
                self.assertEqual(self.events, ["discover", "audit"])
                self.assertEqual(self.read()["sourceState"], source)
                self.assertEqual(self.read()["retryAfter"] is not None, source == "BLOCKED")
                self.assertEqual(self.snapshot.read_bytes(), b"last valid snapshot")

    def test_stale_source_report_is_not_reused_when_child_dies(self):
        report = self.state / "source-access-result.json"
        report.write_text(json.dumps({"schemaVersion": 1, "checkedAt": self.now.isoformat(),
                                      "sourceState": "BLOCKED", "reason": "YOJ_TLS_CERTIFICATE_ERROR", "snapshotChanged": None}))
        original = self.command
        with patch.object(scheduler, "run_command", side_effect=lambda command, *args: 1 if Path(command[1]).name == "audit_online_availability.py" else original(command, *args)):
            self.assertEqual(scheduler.run_once(self.args), 3)
        self.assertEqual(self.read()["sourceState"], "ERROR")
        self.assertIsNone(self.read()["retryAfter"])

    def test_non_nightly_manual_mode_retains_immediate_source_check(self):
        self.audit_failure = ("YOJ_TLS_CERTIFICATE_ERROR", "BLOCKED")
        self.assertEqual(scheduler.run_once(self.args), 3)
        self.audit_failure = None
        self.now = at("2026-10-05T22:45:00+08:00")
        self.events.clear()
        self.args.push = False
        self.assertEqual(scheduler.run_once(self.args), 0)
        self.assertEqual(self.events, ["discover", "audit"])
        self.assertEqual(self.read()["syncState"], "BLOCKED")  # Manual work cannot manufacture a nightly success.

    def test_missing_account_keeps_existing_partial_flow_without_claiming_success(self):
        last = self.successful_baseline()
        self.now = at("2026-10-05T22:45:00+08:00")
        with patch.object(scheduler, "select_pending_personal_ac_targets", return_value=[1717]), \
             patch.object(scheduler, "prepare_online_environment", side_effect=RuntimeError("fixture unavailable")):
            self.assertEqual(scheduler.run_once(self.args), 3)
        self.assertEqual(self.read()["sourceState"], "AVAILABLE")
        self.assertEqual(self.read()["syncState"], "INCOMPLETE")
        self.assertEqual(self.read()["reason"], "YOJ_AUTH_REQUIRED")
        self.assertEqual(self.read()["lastSuccessfulSyncAt"], last)

    def test_submission_gate_or_login_skip_cannot_advance_success_even_on_exit_zero(self):
        last = self.successful_baseline()
        for missing_account in (False, True):
            self.now = at("2026-10-05T22:45:00+08:00")
            with patch.object(scheduler, "select_raw_cleanup_backlog", return_value=[1717]), \
                 patch.dict(scheduler.os.environ, {"YOJ_SYNC_ENABLE_SUBMIT": "1" if missing_account else "0"}), \
                 patch.object(scheduler, "prepare_online_environment", return_value={"YOJ_LOGIN_USER": "fixture"},
                              side_effect=RuntimeError("fixture unavailable") if missing_account else None):
                self.assertEqual(scheduler.run_once(self.args), 3 if missing_account else 0)
            self.assertEqual(self.read()["syncState"], "INCOMPLETE")
            self.assertEqual(self.read()["lastSuccessfulSyncAt"], last)

    def test_online_failure_retains_partial_publication_and_previous_success(self):
        last = self.successful_baseline()
        original = self.command
        def failed_online(command, *args):
            return 3 if Path(command[1]).name == "online_verify.py" else original(command, *args)
        self.now = at("2026-10-05T22:45:00+08:00")
        with patch.object(scheduler, "select_raw_cleanup_backlog", return_value=[1717]), \
             patch.object(scheduler, "run_command", side_effect=failed_online):
            self.assertEqual(scheduler.run_once(self.args), 3)
        scheduler.publish_verified_checkpoint.assert_called()
        self.assertEqual(self.read()["syncState"], "INCOMPLETE")
        self.assertEqual(self.read()["lastSuccessfulSyncAt"], last)

    def test_maintenance_lock_and_remote_checkpoint_do_not_count_as_sync(self):
        last = self.successful_baseline()
        self.now = at("2026-10-05T22:45:00+08:00")
        with patch.object(scheduler, "in_maintenance", return_value=True):
            self.assertEqual(scheduler.run_once(self.args), 0)
        self.assertEqual(self.read()["lastSuccessfulSyncAt"], last)
        with (self.state / "scheduler.lock").open("a") as held:
            fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
            self.assertEqual(scheduler.run_once(self.args), 0)
        self.assertEqual(self.read()["lastSuccessfulSyncAt"], last)
        with patch.object(scheduler, "verify_pending_remote", return_value=False):
            self.assertEqual(scheduler.run_once(self.args), 8)
        self.assertEqual(self.read()["sourceState"], "UNKNOWN")
        self.assertEqual(self.read()["syncState"], "INCOMPLETE")
        self.assertEqual(self.read()["lastSuccessfulSyncAt"], last)

    def test_locked_attempt_cannot_overwrite_a_concurrent_completed_sync(self):
        self.successful_baseline()
        self.now = at("2026-10-05T22:45:00+08:00")
        def concurrent_success(_args, event, **_kwargs):
            if event == "skipped":
                payload = self.read()
                payload.update(startedAt=self.now.isoformat(), finishedAt=self.now.isoformat(),
                               lastSuccessfulSyncAt=self.now.isoformat())
                status.write(self.state, payload)
        with (self.state / "scheduler.lock").open("a") as held, \
             patch.object(scheduler, "health_observe", side_effect=concurrent_success):
            fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
            self.assertEqual(scheduler.run_once(self.args), 0)
        self.assertEqual(self.read()["syncState"], "SUCCEEDED")
        self.assertEqual(self.read()["lastSuccessfulSyncAt"], self.now.isoformat())

    def test_status_write_keeps_success_time_monotonic_and_rejects_older_completion(self):
        self.successful_baseline()
        older = self.read()
        self.now = at("2026-10-05T22:45:00+08:00")
        self.successful_baseline()
        current = self.read()
        status.write(self.state, older)
        self.assertEqual(self.read(), current)
        later = dict(older, startedAt="2026-10-05T22:46:00+08:00", finishedAt="2026-10-05T22:46:00+08:00",
                     syncState="INCOMPLETE", reason="SYNC_INCOMPLETE")
        status.write(self.state, later)
        self.assertEqual(self.read()["syncState"], "INCOMPLETE")
        self.assertEqual(self.read()["lastSuccessfulSyncAt"], current["lastSuccessfulSyncAt"])

    def test_dry_run_and_visibility_watch_do_not_advance_main_success(self):
        last = self.successful_baseline()
        self.now = at("2026-10-05T22:45:00+08:00")
        self.args.dry_run = True
        self.assertEqual(scheduler.run_once(self.args), 0)
        self.assertEqual(self.read()["lastSuccessfulSyncAt"], last)
        self.args.dry_run = False
        self.args.allow_submit = False
        self.args.visibility_watch = True
        self.assertEqual(scheduler.run_once(self.args), 0)
        self.assertEqual(self.read()["mode"], "VISIBILITY")
        self.assertEqual(self.read()["lastSuccessfulSyncAt"], last)

    def test_observer_write_failure_preserves_original_result(self):
        with patch.object(status, "write", side_effect=OSError("fixture unavailable")):
            self.assertEqual(scheduler.run_once(self.args), 0)
        self.assertFalse((self.state / "sync-status.json").exists())


if __name__ == "__main__":
    unittest.main()
