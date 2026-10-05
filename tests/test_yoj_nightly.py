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

from tools import yoj_scheduler as scheduler, yoj_sync_status as status


def at(value="2026-10-05T22:30:00+08:00"):
    return datetime.fromisoformat(value)


class NightlyTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.state = self.root / ".yoj-sync"
        self.state.mkdir()
        self.snapshot = self.root / "public.json"
        self.snapshot.write_bytes(b"last valid snapshot")
        self.now = at()
        self.events = []
        self.audit_failure = None
        self.discovery = {"scope": "public_problem_numbers_only", "newProblemNumbers": []}
        self.capture = {
            "scope": "public_problem_numbers_and_personal_accepted", "newProblemNumbers": [],
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
            "prepare_online_environment": {}, "publish_verified_checkpoint": 0,
            "remember_generated_changes": None, "health_observe": None, "log": None,
        }.items():
            self.stack.enter_context(patch.object(scheduler, name, return_value=value))
        self.stack.enter_context(patch.object(scheduler, "now_local", side_effect=lambda: self.now))
        self.stack.enter_context(patch.object(scheduler, "run_command", side_effect=self.command))
        self.stack.enter_context(patch.object(scheduler, "load_capture_result", side_effect=lambda: self.discovery if self.events[-1] == "discover" else self.capture))
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
        self.assertEqual(self.events, ["audit", "discover"])
        self.assertEqual(self.read()["sourceState"], "AVAILABLE")
        self.assertEqual(self.read()["lastSuccessfulSyncAt"], self.now.isoformat())
        self.assertEqual(self.snapshot.read_bytes(), b"last valid snapshot")
        scheduler.publish_verified_checkpoint.assert_not_called()
        scheduler.prepare_online_environment.assert_not_called()

    def test_guard_invocation_binding_is_recorded_only_in_private_status(self):
        invocation = "a" * 32
        with patch.dict(scheduler.os.environ, {"YOJ_SYNC_INVOCATION_ID": invocation}):
            self.successful_baseline()
        self.assertEqual(self.read()["invocationId"], invocation)

    def test_selected_success_keeps_existing_gate_order_and_publication(self):
        with patch.object(scheduler, "select_pending_personal_ac_targets", return_value=[1717]):
            self.successful_baseline()
        gates = ["build_initial.py", "normalize_cpp_headers.py", "port_cpp17_candidates.py",
                 "repair_known_candidates.py", "sanitize_candidates.py", "validate_candidates.py",
                 "compile_candidates.py", "run_samples.py", "audit_forms.py", "audit_consistency.py"]
        self.assertEqual(self.events, ["audit", "discover", "capture", *gates, "online_verify.py"])
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
        self.assertEqual(self.events, ["release"])
        scheduler.verify_pending_remote.assert_called()
        self.assertEqual(self.read()["retryAfter"], deadline)
        self.assertEqual(self.read()["lastSuccessfulSyncAt"], last)
        self.assertEqual(self.snapshot.read_bytes(), b"last valid snapshot")
        self.now = at("2026-10-05T23:01:00+08:00")
        self.audit_failure = None
        self.events.clear()
        self.assertEqual(scheduler.run_once(self.args), 0)
        self.assertEqual(self.events, ["audit", "discover"])
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
                self.assertEqual(self.events, ["audit"])
                self.assertEqual(self.read()["sourceState"], source)
                self.assertEqual(self.read()["retryAfter"] is not None, source == "BLOCKED")
                self.assertEqual(self.snapshot.read_bytes(), b"last valid snapshot")

    def test_stale_source_report_is_not_reused_when_child_dies(self):
        report = self.state / "source-access-result.json"
        report.write_text(json.dumps({"schemaVersion": 1, "checkedAt": self.now.isoformat(),
                                      "sourceState": "BLOCKED", "reason": "YOJ_TLS_CERTIFICATE_ERROR", "snapshotChanged": None}))
        with patch.object(scheduler, "run_command", return_value=1):
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
        self.assertEqual(self.events, ["audit", "discover"])
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
                 patch.object(scheduler, "prepare_online_environment", side_effect=RuntimeError("fixture unavailable")):
                self.assertEqual(scheduler.run_once(self.args), 0)
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
