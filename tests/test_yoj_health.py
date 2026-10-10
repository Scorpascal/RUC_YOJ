"""Offline tests of receipt truthfulness, privacy, Git isolation and fail-open hooks."""

from __future__ import annotations

import argparse
import base64
import copy
import fcntl
import json
import os
import subprocess
import tempfile
import unittest
import urllib.error
from datetime import datetime
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from tools import yoj_health as health, yoj_launchd_guard as guard, yoj_scheduler as scheduler


def at(value: str = "2026-10-03T23:56:00+08:00") -> datetime:
    return datetime.fromisoformat(value)


def receipt(cycle: str = "2026-10-03", outcome: str = "RETURNED", code: int | None = 0) -> dict:
    return {"schemaVersion": 1, "cycleDate": cycle, "attempts": [{
        "attemptNo": 1, "invokedAt": cycle + "T22:30:00+08:00",
        "enteredAt": cycle + "T22:30:01+08:00",
        "finishedAt": cycle + "T22:40:00+08:00" if outcome != "RUNNING" else None,
        "outcome": outcome, "exitCode": code if outcome != "RUNNING" else None, "skipReason": None,
    }]}


def arguments(**overrides) -> argparse.Namespace:
    values = dict(allow_submit=True, publish=True, push=True, dry_run=False,
                  visibility_watch=False, drift_audit=False, sentinel=False)
    return argparse.Namespace(**{**values, **overrides})


def sync_evidence(source="AVAILABLE", state="SUCCEEDED", reason="NONE",
                  successful="2026-10-03T22:40:00+08:00") -> dict:
    return {"sourceState": source, "syncState": state, "reason": reason,
            "lastSuccessfulSyncAt": successful}


def sync_status(**overrides) -> dict:
    return {"schemaVersion": 1, "startedAt": "2026-10-03T22:30:00+08:00",
            "finishedAt": "2026-10-03T22:40:00+08:00", "mode": "MAIN",
            "retryAfter": None, **sync_evidence(), **overrides}


class ReceiptTests(unittest.TestCase):
    def test_no_change_and_business_failure_both_prove_scheduled_execution(self):
        for code, run in [(0, "RETURNED_OK"), (3, "RETURNED_NONZERO"), (4, "RETURNED_NONZERO"), (8, "RETURNED_NONZERO")]:
            with self.subTest(code=code):
                result = health.classify(receipt(code=code), "2026-10-03", at())
                self.assertEqual((result["scheduleState"], result["runState"]), ("ON_TIME", run))
                self.assertEqual((result["sourceState"], result["syncState"]), ("UNKNOWN", "UNKNOWN"))
                self.assertIsNone(result["lastSuccessfulSyncAt"])

    def test_explicit_success_including_no_change_is_separate_from_scheduling(self):
        value = receipt()
        value["attempts"][0]["sync"] = sync_evidence()
        result = health.classify(value, value["cycleDate"], at())
        self.assertEqual((result["scheduleState"], result["sourceState"], result["syncState"]),
                         ("ON_TIME", "AVAILABLE", "SUCCEEDED"))
        self.assertEqual(result["lastSuccessfulSyncAt"], value["attempts"][0]["finishedAt"])

    def test_blocked_and_incomplete_sources_do_not_become_sync_success(self):
        for source, state, reason in [("BLOCKED", "BLOCKED", "YOJ_TLS_CERTIFICATE_ERROR"),
                                     ("AVAILABLE", "INCOMPLETE", "YOJ_AUTH_REQUIRED"),
                                     ("TRANSIENT_ERROR", "BLOCKED", "YOJ_TRANSIENT_NETWORK_ERROR")]:
            with self.subTest(reason=reason):
                value = receipt(code=3)
                value["attempts"][0]["sync"] = sync_evidence(source, state, reason, "2026-10-02T22:40:00+08:00")
                result = health.classify(value, value["cycleDate"], at())
                self.assertEqual((result["scheduleState"], result["sourceState"], result["syncState"]),
                                 ("ON_TIME", source, state))
                self.assertEqual(result["lastSuccessfulSyncAt"], "2026-10-02T22:40:00+08:00")

    def test_optional_sync_rejects_private_unknown_or_unproven_success(self):
        mutations = [lambda a: a["sync"].update(url="PRIVATE_MARKER"),
                     lambda a: a["sync"].update(reason="PRIVATE_MARKER"),
                     lambda a: a["sync"].update(sourceState=[]),
                     lambda a: a["sync"].update(syncState="UNKNOWN"),
                     lambda a: a["sync"].update(lastSuccessfulSyncAt="2026-10-03T22:40:00Z"),
                     lambda a: a["sync"].update(lastSuccessfulSyncAt="2026-10-03T22:40:01+08:00"),
                     lambda a: a["sync"].update(lastSuccessfulSyncAt="2026-10-02T22:40:00+08:00"),
                     lambda a: a["sync"].update(lastSuccessfulSyncAt=None),
                     lambda a: a["sync"].update(sourceState="BLOCKED"),
                     lambda a: a.update(enteredAt=None), lambda a: a.update(exitCode=3),
                     lambda a: a.update(outcome="RUNNING", finishedAt=None, exitCode=None)]
        for mutate in mutations:
            value = receipt()
            attempt = value["attempts"][0]
            attempt["sync"] = sync_evidence()
            mutate(attempt)
            with self.subTest(value=value), self.assertRaises(health.HealthError):
                health.validate(value, value["cycleDate"], at())

    def test_latest_unknown_attempt_does_not_erase_previous_success_time(self):
        value = receipt()
        value["attempts"][0]["sync"] = sync_evidence()
        second = copy.deepcopy(receipt(outcome="RUNNING")["attempts"][0])
        second.update(attemptNo=2, invokedAt="2026-10-03T22:45:00+08:00", enteredAt="2026-10-03T22:45:01+08:00")
        value["attempts"].append(second)
        result = health.classify(value, value["cycleDate"], at())
        self.assertEqual(result["syncState"], "UNKNOWN")
        self.assertEqual(result["lastSuccessfulSyncAt"], "2026-10-03T22:40:00+08:00")

    def test_missing_terminal_does_not_guess_alive_or_dead(self):
        result = health.classify(receipt(outcome="RUNNING"), "2026-10-03", at())
        self.assertEqual(result["scheduleState"], "ON_TIME")
        self.assertEqual(result["runState"], "NO_TERMINAL_RECEIPT")

    def test_skipped_zero_exit_and_unconfirmed_entry_are_not_execution(self):
        value = receipt()
        attempt = value["attempts"][0]
        attempt["enteredAt"] = None
        self.assertEqual(health.classify(value, value["cycleDate"], at())["scheduleState"], "ENTRY_UNCONFIRMED")
        for reason in ("LOCKED", "MAINTENANCE"):
            attempt.update(outcome="SKIPPED", skipReason=reason)
            self.assertEqual(health.classify(value, value["cycleDate"], at())["scheduleState"], "NOT_ENTERED")

    def test_schedule_start_and_end_boundaries(self):
        for value, expected in [("22:29:59", "OUT_OF_WINDOW"), ("22:30:00", "ON_TIME"),
                                ("23:29:59", "ON_TIME"), ("23:30:00", "OUT_OF_WINDOW")]:
            with self.subTest(time=value):
                payload = receipt()
                attempt = payload["attempts"][0]
                attempt.update(invokedAt=f"2026-10-03T{value}+08:00", enteredAt=f"2026-10-03T{value}+08:00",
                               finishedAt="2026-10-03T23:50:00+08:00")
                self.assertEqual(health.classify(payload, "2026-10-03", at())["scheduleState"], expected)

    def test_cross_midnight_finish_keeps_original_cycle(self):
        payload = receipt()
        payload["attempts"][0]["finishedAt"] = "2026-10-04T00:15:00+08:00"
        self.assertEqual(health.classify(payload, "2026-10-03", at("2026-10-04T00:20:00+08:00"))["scheduleState"], "ON_TIME")

    def test_retries_keep_first_execution_and_all_results(self):
        value = receipt(code=8)
        second = copy.deepcopy(value["attempts"][0])
        second.update(attemptNo=2, invokedAt="2026-10-03T22:45:00+08:00", enteredAt="2026-10-03T22:45:01+08:00",
                      finishedAt="2026-10-03T22:50:00+08:00", exitCode=0)
        value["attempts"].append(second)
        result = health.classify(value, value["cycleDate"], at())
        self.assertEqual(result["scheduleState"], "ON_TIME")
        self.assertEqual(result["runState"], "RETURNED_OK")
        self.assertEqual([a["exitCode"] for a in result["attempts"]], [8, 0])

    def test_private_unknown_and_invalid_fields_rejected(self):
        mutations = [
            lambda p: p.update(username="PRIVATE_MARKER"),
            lambda p: p["attempts"][0].update(error="PRIVATE_MARKER"),
            lambda p: p.update(schemaVersion=True),
            lambda p: p.update(cycleDate="2026-10-02"),
            lambda p: p["attempts"][0].update(attemptNo=True),
            lambda p: p["attempts"][0].update(exitCode=True),
            lambda p: p["attempts"][0].update(invokedAt="2026-10-03T22:30:00Z"),
            lambda p: p["attempts"][0].update(enteredAt="2026-10-03T22:29:00+08:00"),
            lambda p: p["attempts"][0].update(finishedAt="2026-10-05T22:30:00+08:00"),
            lambda p: p["attempts"][0].update(outcome="PRIVATE_MARKER"),
            lambda p: p["attempts"].append(copy.deepcopy(p["attempts"][0])),
        ]
        for mutate in mutations:
            value = receipt()
            mutate(value)
            with self.subTest(value=value), self.assertRaises(health.HealthError):
                health.validate(value, "2026-10-03", at())
        with self.assertRaises(health.HealthError):
            health.decode(b" " * (health.MAX_BYTES + 1), "2026-10-03")
        raw = health.encode(receipt()).replace(b'"schemaVersion":1', b'"schemaVersion":1,"schemaVersion":1')
        with self.assertRaises(health.HealthError):
            health.decode(raw, "2026-10-03")

    def test_merge_never_downgrades_terminal_or_invents_admission(self):
        running, done = receipt(outcome="RUNNING"), receipt()
        self.assertEqual(health.merge_receipts(running, done), done)
        self.assertEqual(health.merge_receipts(done, running), done)
        with self.assertRaises(health.HealthError):
            health.merge_receipts(done, receipt(code=8))
        unrelated = receipt()
        unrelated["attempts"][0]["invokedAt"] = "2026-10-03T22:30:01+08:00"
        with self.assertRaises(health.HealthError):
            health.merge_receipts(done, unrelated)

    def test_optional_sync_can_only_extend_an_identical_terminal_receipt(self):
        old = receipt()
        new = copy.deepcopy(old)
        new["attempts"][0]["sync"] = sync_evidence()
        self.assertEqual(health.merge_receipts(old, new), new)
        self.assertEqual(health.merge_receipts(new, old), new)
        self.assertEqual(health.merge_receipts(new, new), new)
        changed = copy.deepcopy(new)
        changed["attempts"][0]["sync"]["lastSuccessfulSyncAt"] = "2026-10-03T22:39:00+08:00"
        with self.assertRaises(health.HealthError):
            health.merge_receipts(new, changed)

    def test_due_date_uses_beijing_deadline_not_elapsed_24_hours(self):
        for value, expected in [
            ("2026-10-03T23:54:59+08:00", ["2026-10-02", "2026-10-01"]),
            ("2026-10-03T23:55:00+08:00", ["2026-10-03", "2026-10-02"]),
            ("2026-10-03T16:20:00+00:00", ["2026-10-03", "2026-10-02"]),
            ("2026-10-04T22:45:00+08:00", ["2026-10-03", "2026-10-02"]),
            ("2026-10-05T00:20:00+08:00", ["2026-10-04", "2026-10-03"]),
        ]:
            with self.subTest(now=value):
                self.assertEqual(health.expected_cycles(at(value), "2026-10-01"), expected)
        self.assertEqual(health.expected_cycles(at("2026-10-03T21:00:00+08:00"), "2026-10-03"), [])
        self.assertEqual(health.expected_cycles(at(), "2026-10-03", "2026-10-02"), [])
        with self.assertRaises(health.HealthError):
            health.expected_cycles(at(), "2026-10-03", "2026-10-04")


class LocalHookTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.clock = patch.object(health, "now_local", return_value=at("2026-10-03T22:30:00+08:00")).start()
        self.addCleanup(patch.stopall)
        self.delivery = patch.object(health, "spawn_delivery").start()

    def read(self):
        return health.read_receipt(health.health_dir(self.root) / "receipts/2026-10-03.json")

    def test_admission_and_completion_are_durable_without_network(self):
        token = health.observe(self.root, "begin")
        self.assertIsNotNone(token)
        self.delivery.assert_not_called()
        health.observe(self.root, "entered", token=token)
        self.assertIsNotNone(self.read()["attempts"][0]["enteredAt"])
        self.delivery.side_effect = OSError("PRIVATE_MARKER")
        health.observe(self.root, "finished", token=token, returncode=8)
        payload = self.read()
        self.assertEqual(payload["attempts"][0]["exitCode"], 8)
        self.assertNotIn("PRIVATE_MARKER", health.encode(payload).decode())

    def test_sync_evidence_is_optional_and_invalid_evidence_does_not_lose_completion(self):
        token = health.observe(self.root, "begin")
        health.observe(self.root, "entered", token=token)
        self.clock.return_value = at("2026-10-03T22:40:00+08:00")
        health.observe(self.root, "finished", token=token, returncode=0, sync=sync_evidence())
        self.assertEqual(self.read()["attempts"][0]["sync"], sync_evidence())
        token = health.observe(self.root, "begin")
        health.observe(self.root, "entered", token=token)
        invalid = {**sync_evidence(), "token": "PRIVATE_MARKER"}
        health.observe(self.root, "finished", token=token, returncode=0, sync=invalid)
        attempt = self.read()["attempts"][-1]
        self.assertEqual((attempt["outcome"], attempt["exitCode"]), ("RETURNED", 0))
        self.assertNotIn("sync", attempt)
        self.assertNotIn("PRIVATE_MARKER", health.encode(self.read()).decode())

    def test_unentered_success_evidence_cannot_manufacture_sync_success(self):
        token = health.observe(self.root, "begin")
        self.clock.return_value = at("2026-10-03T22:40:00+08:00")
        health.observe(self.root, "finished", token=token, returncode=0, sync=sync_evidence())
        attempt = self.read()["attempts"][0]
        self.assertEqual(attempt["outcome"], "RETURNED")
        self.assertNotIn("sync", attempt)

    def test_returned_zero_does_not_overwrite_lock_skip(self):
        token = health.observe(self.root, "begin")
        health.observe(self.root, "skipped", token=token, reason="LOCKED")
        health.observe(self.root, "finished", token=token, returncode=0)
        attempt = self.read()["attempts"][0]
        self.assertEqual(attempt["outcome"], "SKIPPED")
        self.assertIsNone(attempt["enteredAt"])

    def test_local_failures_never_propagate(self):
        with patch.object(health, "atomic_json", side_effect=OSError("PRIVATE_MARKER")):
            self.assertIsNone(health.observe(self.root, "begin"))
        self.assertIsNone(health.observe(self.root, "entered", token="../private"))
        self.clock.return_value = at("2026-10-03T23:30:00+08:00")
        self.assertIsNone(health.observe(self.root, "begin"))

    def test_spawn_failure_record_and_signal_exit(self):
        for code in (None, -15):
            token = health.observe(self.root, "begin")
            health.observe(self.root, "finished", token=token, returncode=code)
            self.assertEqual(self.read()["attempts"][-1]["outcome"], "PROCESS_ERROR")

    def test_guard_preserves_result_command_and_main_state(self):
        for code in (0, 3, 8):
            with self.subTest(code=code), patch.object(guard, "ROOT", self.root), patch.object(guard, "log"), \
                 patch.object(guard.subprocess, "run", return_value=SimpleNamespace(returncode=code)) as run:
                self.assertEqual(guard.run_scheduler(), code)
                command = run.call_args.args[0]
                self.assertEqual(command[-3:], ["--allow-submit", "--publish", "--push"])
                self.assertIn(health.CONTEXT_ENV, set(run.call_args.kwargs["env"]))
                self.assertFalse((self.root / ".yoj-sync/launchd-cycle.json").exists())
        with patch.object(guard, "ROOT", self.root), patch.object(guard, "log"), \
             patch.object(health, "observe", side_effect=RuntimeError("observer unavailable")), \
             patch.object(guard.subprocess, "run", return_value=SimpleNamespace(returncode=8)):
            self.assertEqual(guard.run_scheduler(), 8)

    def test_guard_process_start_exception_still_propagates(self):
        with patch.object(guard, "ROOT", self.root), patch.object(guard.subprocess, "run", side_effect=OSError("process unavailable")):
            with self.assertRaises(OSError):
                guard.run_scheduler()
        self.assertEqual(self.read()["attempts"][0]["outcome"], "PROCESS_ERROR")

    def test_visibility_and_non_nightly_modes_cannot_manufacture_main_receipt(self):
        with patch.object(guard, "ROOT", self.root), patch.object(guard, "log"), \
             patch.dict(os.environ, {health.CONTEXT_ENV: "stale"}), \
             patch.object(guard.subprocess, "run", return_value=SimpleNamespace(returncode=0)) as run:
            guard.run_scheduler(visibility_watch=True)
            self.assertNotIn(health.CONTEXT_ENV, set(run.call_args.kwargs["env"]))
        self.assertFalse((health.health_dir(self.root) / "receipts").exists())
        with patch.dict(os.environ, {health.CONTEXT_ENV: "a" * 32}), patch.object(health, "observe") as observer:
            for mode in ("visibility_watch", "dry_run", "sentinel", "drift_audit"):
                scheduler.health_observe(arguments(**{mode: True}), "entered")
            scheduler.health_observe(arguments(push=False), "entered")
            observer.assert_not_called()

    def test_scheduler_real_lock_conflict_has_no_admission_even_with_zero_exit(self):
        token = health.observe(self.root, "begin")
        lock_path = self.root / "scheduler.lock"
        with lock_path.open("a") as held:
            fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
            with patch.object(scheduler, "ROOT", self.root), patch.object(scheduler, "STATE_DIR", self.root), \
                 patch.object(scheduler, "LOCK_PATH", lock_path), patch.object(scheduler, "in_maintenance", return_value=False), \
                 patch.object(scheduler, "log"), patch.dict(os.environ, {health.CONTEXT_ENV: token}):
                self.assertEqual(scheduler.run_once(arguments()), 0)
        self.assertEqual(self.read()["attempts"][0]["skipReason"], "LOCKED")

    def test_admission_observer_does_not_bypass_dirty_worktree_gate(self):
        token = health.observe(self.root, "begin")
        with patch.object(scheduler, "ROOT", self.root), patch.object(scheduler, "STATE_DIR", self.root), \
             patch.object(scheduler, "LOCK_PATH", self.root / "scheduler.lock"), \
             patch.object(scheduler, "in_maintenance", return_value=False), \
             patch.object(scheduler, "validate_worktree_before_run", return_value=False), \
             patch.object(scheduler, "verify_pending_remote") as release, \
             patch.dict(os.environ, {health.CONTEXT_ENV: token}):
            self.assertEqual(scheduler.run_once(arguments()), 4)
            release.assert_not_called()
        self.assertIsNotNone(self.read()["attempts"][0]["enteredAt"])

    def test_delivery_environment_drops_credentials_and_custom_git_overrides(self):
        with patch.dict(os.environ, {"YOJ_LOGIN_PASS": "PRIVATE_MARKER", "GH_TOKEN": "PRIVATE_MARKER",
                                    "GIT_CONFIG_COUNT": "1", "YOJ_LOGIN_USER": "PRIVATE_MARKER"}):
            environment = health.clean_environment()
            self.assertNotIn("PRIVATE_MARKER", str(environment))
            self.assertNotIn("GIT_CONFIG_COUNT", environment)

    def test_buffer_retries_receipts_only_and_maintenance_remains_quiet(self):
        for moment, event in [("23:30:00", "flush"), ("23:45:00", "flush"),
                              ("23:54:59", "flush"), ("23:55:00", None),
                              ("00:05:00", None), ("00:10:00", "retry-pending"),
                              ("01:41:00", "retry-pending"), ("21:00:00", "retry-pending")]:
            with self.subTest(moment=moment), patch.object(guard, "now_local", return_value=at(f"2026-10-04T{moment}+08:00")), \
                 patch.object(guard, "health_observe") as observer, patch.object(guard, "run_scheduler") as run, \
                 patch.object(guard, "save_state") as save, patch.object(guard, "log"):
                self.assertEqual(guard.main(), 0)
                self.assertEqual(observer.call_args_list, [unittest.mock.call(event)] if event else [])
                run.assert_not_called()
                save.assert_not_called()


class GuardStatusTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.directory = self.root / ".yoj-sync"
        self.directory.mkdir()
        self.path = self.directory / guard.SYNC_STATUS_NAME
        self.started = at("2026-10-03T22:30:00+08:00")
        self.finished = at("2026-10-03T22:40:00+08:00")
        for name, value in [("ROOT", self.root), ("STATE_DIR", self.directory),
                            ("GUARD_LOCK_PATH", self.directory / "launchd-guard.lock")]:
            patch.object(guard, name, value).start()
        self.guard_clock = patch.object(guard, "now_local", return_value=self.started).start()
        self.health_clock = patch.object(health, "now_local", return_value=self.started).start()
        patch.object(health, "spawn_delivery").start()
        patch.object(guard, "log").start()
        self.addCleanup(patch.stopall)

    def write(self, payload=None, *, raw=None, modified=None):
        self.path.write_bytes(raw if raw is not None else health.encode(payload or sync_status()))
        moment = (modified or self.finished).timestamp()
        os.utime(self.path, (moment, moment))

    def test_fresh_full_success_status_and_return_code_are_both_required(self):
        self.write()
        self.assertEqual(guard.read_current_sync(self.started, self.finished, 0), sync_evidence())
        for code in (3, -15, None):
            self.assertIsNone(guard.read_current_sync(self.started, self.finished, code))

    def test_current_invocation_id_is_required_when_guard_supplies_it(self):
        invocation = "a" * 32
        self.write()
        self.assertIsNone(guard.read_current_sync(self.started, self.finished, 0, invocation_id=invocation))
        self.write(sync_status(invocationId=invocation))
        self.assertEqual(guard.read_current_sync(self.started, self.finished, 0, invocation_id=invocation), sync_evidence())
        self.assertIsNone(guard.read_current_sync(self.started, self.finished, 0, invocation_id="b" * 32))
        self.write(sync_status(invocationId="invalid"))
        self.assertIsNone(guard.read_current_sync(self.started, self.finished, 0))

    def test_status_rejects_stale_oversized_symlink_nonregular_or_invalid_payload(self):
        cases = [sync_status(mode="VISIBILITY"), sync_status(schemaVersion=True),
                 sync_status(token="PRIVATE_MARKER"), sync_status(reason="PRIVATE_MARKER"),
                 sync_status(startedAt="2026-10-03T22:29:59+08:00"),
                 sync_status(finishedAt="2026-10-03T22:40:01+08:00"),
                 sync_status(lastSuccessfulSyncAt="2026-10-03T22:39:00+08:00"),
                 sync_status(retryAfter="2026-10-03T22:35:00+08:00")]
        for payload in cases:
            with self.subTest(payload=payload):
                self.write(payload)
                self.assertIsNone(guard.read_current_sync(self.started, self.finished, 0))
        self.write(modified=at("2026-10-03T22:29:59+08:00"))
        self.assertIsNone(guard.read_current_sync(self.started, self.finished, 0))
        self.write(raw=b" " * (guard.MAX_STATUS_BYTES + 1))
        self.assertIsNone(guard.read_current_sync(self.started, self.finished, 0))
        self.write(raw=health.encode(sync_status()).replace(b'"mode":"MAIN"', b'"mode":"MAIN","mode":"MAIN"'))
        self.assertIsNone(guard.read_current_sync(self.started, self.finished, 0))
        target = self.root / "status.json"
        self.path.replace(target)
        self.path.symlink_to(target)
        self.assertIsNone(guard.read_current_sync(self.started, self.finished, 0))
        self.path.unlink()
        os.mkfifo(self.path)
        self.assertIsNone(guard.read_current_sync(self.started, self.finished, 0))

    def test_guard_attaches_only_this_run_evidence_and_preserves_business_exit(self):
        for code, state, source, reason in [(0, "SUCCEEDED", "AVAILABLE", "NONE"),
                                            (3, "BLOCKED", "BLOCKED", "YOJ_TLS_CERTIFICATE_ERROR")]:
            with self.subTest(code=code):
                if code == 3:
                    self.started, self.finished = at("2026-10-03T22:45:00+08:00"), at("2026-10-03T22:50:00+08:00")
                self.guard_clock.return_value = self.started
                self.health_clock.return_value = self.started
                successful = "2026-10-03T22:40:00+08:00" if code == 0 else "2026-10-02T22:40:00+08:00"
                payload = sync_status(startedAt=health.stamp(self.started), finishedAt=health.stamp(self.finished),
                                      syncState=state, sourceState=source, reason=reason, lastSuccessfulSyncAt=successful)
                def run(*args, **kwargs):
                    health.observe(self.root, "entered", token=kwargs["env"][health.CONTEXT_ENV])
                    payload["invocationId"] = kwargs["env"][guard.INVOCATION_ENV]
                    self.write(payload)
                    self.guard_clock.return_value = self.finished
                    self.health_clock.return_value = self.finished
                    return SimpleNamespace(returncode=code)
                with patch.object(guard.subprocess, "run", side_effect=run):
                    self.assertEqual(guard.run_scheduler(), code)
                journal = health.read_receipt(health.health_dir(self.root) / "receipts/2026-10-03.json")
                self.assertEqual(journal["attempts"][-1]["sync"], {key: payload[key] for key in health.SYNC_KEYS})
                self.assertNotIn(payload["invocationId"], health.encode(journal).decode())

    def test_stale_or_io_failure_does_not_change_exit_and_does_not_create_sync_evidence(self):
        self.write(modified=at("2026-10-03T22:29:59+08:00"))
        def run(*args, **kwargs):
            health.observe(self.root, "entered", token=kwargs["env"][health.CONTEXT_ENV])
            self.guard_clock.return_value = self.finished
            self.health_clock.return_value = self.finished
            return SimpleNamespace(returncode=3)
        with patch.object(guard.subprocess, "run", side_effect=run):
            self.assertEqual(guard.run_scheduler(), 3)
        journal = health.read_receipt(health.health_dir(self.root) / "receipts/2026-10-03.json")
        self.assertNotIn("sync", journal["attempts"][0])
        with patch.object(guard, "read_bounded_object", side_effect=OSError("PRIVATE_MARKER")), \
             patch.object(guard.subprocess, "run", return_value=SimpleNamespace(returncode=8)):
            self.assertEqual(guard.run_scheduler(), 8)

    def test_checkpoint_accepts_explicit_success_or_confirmed_legacy_entry_only(self):
        cases = [(0, sync_evidence(), True, True, True),
                 (0, sync_evidence("UNKNOWN", "SKIPPED", "SYNC_SKIPPED", None), True, False, False),
                 (0, sync_evidence("AVAILABLE", "INCOMPLETE", "SYNC_INCOMPLETE", None), True, True, False),
                 (3, sync_evidence("BLOCKED", "BLOCKED", "YOJ_TLS_CERTIFICATE_ERROR", None), True, True, False),
                 (0, None, True, True, False), (0, None, False, True, True), (0, None, False, False, False)]
        for code, evidence, has_status, admitted, succeeds in cases:
            with self.subTest(code=code, evidence=evidence, has_status=has_status, admitted=admitted):
                def run(visibility_watch=False):
                    self.assertFalse(visibility_watch)
                    guard._last_run_sync, guard._last_run_admitted, guard._last_run_has_status = evidence, admitted, has_status
                    return code
                with patch.object(guard, "load_state", return_value={}), patch.object(guard, "run_scheduler", side_effect=run), \
                     patch.object(guard, "save_state") as save:
                    self.assertEqual(guard.main(), code)
                    self.assertEqual(save.called, succeeds)
                    if succeeds:
                        self.assertEqual(save.call_args.args[0]["lastSuccessfulCycle"], "2026-10-03")
                        if evidence:
                            self.assertEqual(save.call_args.args[0]["lastSuccessfulAt"], evidence["lastSuccessfulSyncAt"])

    def test_actual_scheduler_lock_and_maintenance_zero_do_not_admit_or_checkpoint(self):
        for skip in ("LOCKED", "MAINTENANCE"):
            with self.subTest(skip=skip):
                self.guard_clock.return_value = self.started
                self.health_clock.return_value = self.started
                def run(*args, **kwargs):
                    health.observe(self.root, "skipped", token=kwargs["env"][health.CONTEXT_ENV], reason=skip)
                    return SimpleNamespace(returncode=0)
                with patch.object(guard, "load_state", return_value={}), \
                     patch.object(guard.subprocess, "run", side_effect=run), patch.object(guard, "save_state") as save:
                    self.assertEqual(guard.main(), 0)
                    save.assert_not_called()
                self.assertFalse(guard._last_run_admitted)
                self.assertTrue(guard._last_run_skipped)

    def test_known_lock_skip_rejects_old_fresh_and_same_second_concurrent_success(self):
        cases = [("old", "2026-10-03T22:30:00.100000+08:00", "2026-10-03T22:30:01.300000+08:00",
                  "2026-10-03T22:29:00+08:00", "2026-10-03T22:29:01+08:00", "2026-10-03T22:29:01+08:00"),
                 ("fresh", "2026-10-03T22:45:00.100000+08:00", "2026-10-03T22:45:01.300000+08:00",
                  "2026-10-03T22:45:00+08:00", "2026-10-03T22:45:01+08:00", "2026-10-03T22:45:01.200000+08:00"),
                 ("same-second", "2026-10-03T23:00:00.100000+08:00", "2026-10-03T23:00:00.300000+08:00",
                  "2026-10-03T23:00:00+08:00", "2026-10-03T23:00:00+08:00", "2026-10-03T23:00:00.200000+08:00")]
        for name, start, finish, status_start, status_finish, modified in cases:
            with self.subTest(status=name):
                self.started, self.finished = at(start), at(finish)
                self.guard_clock.return_value = self.started
                self.health_clock.return_value = self.started
                payload = sync_status(startedAt=status_start, finishedAt=status_finish,
                                      lastSuccessfulSyncAt=status_finish)
                self.write(payload, modified=at(modified))
                if name != "old":
                    # These shared times pass freshness checks independently;
                    # the local skip must veto their attribution to this run.
                    self.assertEqual(guard.read_current_sync(self.started, self.finished, 0)["syncState"], "SUCCEEDED")
                def run(*args, **kwargs):
                    health.observe(self.root, "skipped", token=kwargs["env"][health.CONTEXT_ENV], reason="LOCKED")
                    payload["invocationId"] = kwargs["env"][guard.INVOCATION_ENV]
                    self.write(payload, modified=at(modified))
                    self.guard_clock.return_value = self.finished
                    self.health_clock.return_value = self.finished
                    return SimpleNamespace(returncode=0)
                with patch.object(guard, "load_state", return_value={}), \
                     patch.object(guard.subprocess, "run", side_effect=run), patch.object(guard, "save_state") as save:
                    self.assertEqual(guard.main(), 0)
                    save.assert_not_called()
                self.assertTrue(guard._last_run_skipped)
                self.assertIsNone(guard._last_run_sync)
                attempt = health.read_receipt(health.health_dir(self.root) / "receipts/2026-10-03.json")["attempts"][-1]
                self.assertEqual((attempt["outcome"], attempt["skipReason"]), ("SKIPPED", "LOCKED"))
                self.assertNotIn("sync", attempt)

    def test_unknown_entry_keeps_strict_current_success_checkpoint_compatibility(self):
        def run(*args, **kwargs):
            self.write(sync_status(invocationId=kwargs["env"][guard.INVOCATION_ENV]))
            self.guard_clock.return_value = self.finished
            self.health_clock.return_value = self.finished
            return SimpleNamespace(returncode=0)
        with patch.object(guard, "observed_entry", return_value="UNKNOWN"), \
             patch.object(guard, "load_state", return_value={}), \
             patch.object(guard.subprocess, "run", side_effect=run), patch.object(guard, "save_state") as save:
            self.assertEqual(guard.main(), 0)
            save.assert_called_once()
        self.assertFalse(guard._last_run_skipped)
        self.assertEqual(guard._last_run_sync["syncState"], "SUCCEEDED")
        attempt = health.read_receipt(health.health_dir(self.root) / "receipts/2026-10-03.json")["attempts"][-1]
        self.assertNotIn("sync", attempt)  # unentered receipt remains UNKNOWN

    def test_unknown_entry_cannot_borrow_another_same_second_invocation_success(self):
        self.started = at("2026-10-03T22:30:00.100000+08:00")
        self.finished = at("2026-10-03T22:30:00.300000+08:00")
        self.guard_clock.return_value = self.started
        self.health_clock.return_value = self.started
        def run(*args, **kwargs):
            other = "a" * 32 if kwargs["env"][guard.INVOCATION_ENV] != "a" * 32 else "b" * 32
            self.write(sync_status(finishedAt="2026-10-03T22:30:00+08:00",
                                   lastSuccessfulSyncAt="2026-10-03T22:30:00+08:00", invocationId=other),
                       modified=at("2026-10-03T22:30:00.200000+08:00"))
            self.assertEqual(guard.read_current_sync(self.started, self.finished, 0)["syncState"], "SUCCEEDED")
            self.guard_clock.return_value = self.finished
            self.health_clock.return_value = self.finished
            return SimpleNamespace(returncode=0)
        with patch.object(guard, "observed_entry", return_value="UNKNOWN"), \
             patch.object(guard, "load_state", return_value={}), \
             patch.object(guard.subprocess, "run", side_effect=run), patch.object(guard, "save_state") as save:
            self.assertEqual(guard.main(), 0)
            save.assert_not_called()
        self.assertFalse(guard._last_run_skipped)
        self.assertIsNone(guard._last_run_sync)

    def test_scheduler_status_wrapper_guard_and_receipt_agree_without_real_business_commands(self):
        cases = [("22:30:00", "22:40:00", "SUCCEEDED", 0),
                 ("22:45:00", "22:45:01", "SKIPPED", 0),
                 ("23:00:00", "23:01:00", "INCOMPLETE", 0),
                 ("23:15:00", "23:16:00", "BLOCKED", 3)]
        for start, finish, state, code in cases:
            with self.subTest(state=state):
                self.started, self.finished = at(f"2026-10-03T{start}+08:00"), at(f"2026-10-03T{finish}+08:00")
                self.guard_clock.return_value = self.started
                self.health_clock.return_value = self.started
                args = arguments()
                previous_status = self.path.read_bytes() if self.path.exists() else None
                def business(_args):
                    context = scheduler._SYNC_CONTEXT.get()
                    if state == "SKIPPED":
                        scheduler.health_observe(args, "skipped", reason="LOCKED")
                    else:
                        scheduler.health_observe(args, "entered")
                        context.update(admitted=True, sourceState="BLOCKED" if state == "BLOCKED" else "AVAILABLE",
                                       sourceReason="YOJ_TLS_CERTIFICATE_ERROR" if state == "BLOCKED" else "NONE",
                                       complete=state == "SUCCEEDED")
                    self.guard_clock.return_value = self.finished
                    self.health_clock.return_value = self.finished
                    return code
                def process(*_args, **kwargs):
                    with patch.dict(os.environ, kwargs["env"]):
                        result = scheduler.run_once(args)
                    if state != "SKIPPED":
                        moment = self.finished.timestamp()
                        os.utime(self.path, (moment, moment))
                    return SimpleNamespace(returncode=result)
                with patch.object(scheduler, "ROOT", self.root), patch.object(scheduler, "STATE_DIR", self.directory), \
                     patch.object(scheduler, "now_local", side_effect=lambda: self.guard_clock.return_value), \
                     patch.object(scheduler, "_run_once", side_effect=business), \
                     patch.object(guard.subprocess, "run", side_effect=process), \
                     patch.object(guard, "load_state", return_value={}), patch.object(guard, "save_state") as save:
                    self.assertEqual(guard.main(), code)
                    self.assertEqual(save.called, state == "SUCCEEDED")
                journal = health.read_receipt(health.health_dir(self.root) / "receipts/2026-10-03.json")
                latest = journal["attempts"][-1]
                if state == "SKIPPED":
                    self.assertEqual(latest["outcome"], "SKIPPED")
                    self.assertEqual(latest["skipReason"], "LOCKED")
                    self.assertNotIn("sync", latest)
                    self.assertEqual(self.path.read_bytes(), previous_status)
                    self.assertIsNone(guard._last_run_sync)
                else:
                    self.assertEqual(latest["sync"]["syncState"], state)
                    self.assertEqual(latest["sync"]["lastSuccessfulSyncAt"], "2026-10-03T22:40:00+08:00")


class DeliveryTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.root = self.directory / "main"
        self.root.mkdir()
        self.remote = self.directory / "receiver.git"
        self.git("init", "--bare", str(self.remote))
        self.git("init", str(self.root))
        self.git("-C", str(self.root), "config", "user.name", health.AUTHOR)
        self.git("-C", str(self.root), "config", "user.email", health.EMAIL)
        (self.root / "tracked.txt").write_text("public fixture\n")
        (self.root / ".gitignore").write_text(".yoj-sync/\n")
        self.git("-C", str(self.root), "add", "tracked.txt", ".gitignore")
        self.git("-C", str(self.root), "commit", "-m", "test fixture")
        patch.object(health, "REMOTE", str(self.remote)).start()
        patch.object(health, "now_local", return_value=at()).start()
        self.addCleanup(patch.stopall)

    def git(self, *args):
        return subprocess.run(["git", *args], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True).stdout.decode().strip()

    def remote_head(self):
        return self.git("--git-dir", str(self.remote), "rev-parse", health.REF)

    def test_empty_initialization_then_receipt_preserves_owner_and_main_index(self):
        (self.root / "tracked.txt").write_text("PRIVATE_MAIN_STAGED_DATA\n")
        self.git("-C", str(self.root), "add", "tracked.txt")
        def main_state():
            return [self.git("-C", str(self.root), *args) for args in
                    [("rev-parse", "HEAD"), ("write-tree",), ("status", "--porcelain=v1")]]
        before = main_state()
        initial = health.GitDelivery(self.root).publish([], initialize=True)
        self.assertEqual(self.git("--git-dir", str(self.remote), "ls-tree", "-r", initial), "")
        self.assertEqual(self.git("--git-dir", str(self.remote), "rev-list", "--parents", "-n", "1", initial), initial)
        delivered = health.GitDelivery(self.root).publish([receipt()])
        self.assertEqual(before, main_state())
        self.assertEqual(self.git("--git-dir", str(self.remote), "ls-tree", "-r", "--name-only", delivered), "receipts/2026-10-03.json")
        identities = self.git("--git-dir", str(self.remote), "log", "-1", "--format=%an <%ae>|%cn <%ce>", delivered)
        self.assertEqual(identities, f"{health.AUTHOR} <{health.EMAIL}>|{health.AUTHOR} <{health.EMAIL}>")
        self.assertEqual(health.GitDelivery(self.root).publish([receipt()]), delivered)
        self.assertEqual(self.remote_head(), delivered)

    def test_completed_receipt_cannot_be_overwritten_by_delayed_running(self):
        health.GitDelivery(self.root).publish([receipt(outcome="RUNNING")])
        final = health.GitDelivery(self.root).publish([receipt()])
        self.assertEqual(health.GitDelivery(self.root).publish([receipt(outcome="RUNNING")]), final)

    def test_bad_payload_and_symlink_never_reach_remote(self):
        initial = health.GitDelivery(self.root).publish([], initialize=True)
        value = receipt()
        value["private"] = "PRIVATE_MARKER"
        with self.assertRaises(health.HealthError):
            health.GitDelivery(self.root).publish([value])
        self.assertEqual(self.remote_head(), initial)
        link = self.root / "2026-10-03.json"
        link.symlink_to(self.root / "tracked.txt")
        with self.assertRaises(health.HealthError):
            health.read_receipt(link)

    def test_rejected_non_fast_forward_preserves_other_writer(self):
        health.GitDelivery(self.root).publish([receipt(outcome="RUNNING")])
        publisher = health.GitDelivery(self.root)
        actual_git = publisher.git
        other_head = []
        def race(*args, **kwargs):
            if args[0] == "push":
                other_head.append(health.GitDelivery(self.root).publish([receipt("2026-10-02")]))
            return actual_git(*args, **kwargs)
        with patch.object(publisher, "git", side_effect=race), self.assertRaises(health.HealthError):
            publisher.publish([receipt()])
        self.assertEqual(self.remote_head(), other_head[0])
        # A later retry fetches that exact history, then safely adds our result.
        health.GitDelivery(self.root).publish([receipt()])
        paths = self.git("--git-dir", str(self.remote), "ls-tree", "-r", "--name-only", self.remote_head())
        self.assertEqual(paths.splitlines(), ["receipts/2026-10-02.json", "receipts/2026-10-03.json"])

    def test_worker_is_disabled_outside_window_and_without_existing_push_gate(self):
        with patch.dict(os.environ, {"YOJ_SYNC_ALLOW_PUSH": "0"}), self.assertRaises(health.HealthError):
            health.deliver(self.root)
        with patch.dict(os.environ, {"YOJ_SYNC_ALLOW_PUSH": "1"}):
            self.assertIsNone(health.deliver(self.root))  # 23:56 maintenance

    def test_final_receipt_arriving_during_upload_is_coalesced(self):
        path = health.health_dir(self.root) / "receipts/2026-10-03.json"
        health.atomic_json(path, receipt(outcome="RUNNING"))
        calls = []
        def publish(values, initialize):
            calls.append(values)
            if len(calls) == 1:
                health.atomic_json(path, receipt())
            return "a" * 40
        with patch.dict(os.environ, {"YOJ_SYNC_ALLOW_PUSH": "1"}), \
             patch.object(health, "now_local", return_value=at("2026-10-03T22:41:00+08:00")), \
             patch.object(health.GitDelivery, "publish", side_effect=publish):
            health.deliver(self.root)
        self.assertEqual(len(calls), 2)
        self.assertEqual(calls[-1][0]["attempts"][0]["outcome"], "RETURNED")

    def test_background_launcher_does_not_wait_or_forward_private_environment(self):
        with patch.object(health, "now_local", return_value=at("2026-10-03T22:41:00+08:00")), \
             patch.dict(os.environ, {"YOJ_SYNC_ALLOW_PUSH": "1", "YOJ_LOGIN_PASS": "PRIVATE_MARKER"}), \
             patch.object(health.subprocess, "Popen") as child:
            health.spawn_delivery(self.root)
        self.assertTrue(child.call_args.kwargs["start_new_session"])
        self.assertNotIn("YOJ_LOGIN_PASS", set(child.call_args.kwargs["env"]))
        child.return_value.wait.assert_not_called()
        child.return_value.communicate.assert_not_called()

    def test_delivery_timeout_kills_only_its_own_git_process_group(self):
        process = SimpleNamespace(pid=999999, returncode=-9)
        from unittest.mock import Mock
        process.communicate = Mock(side_effect=[subprocess.TimeoutExpired("git", 1), (b"", b"")])
        with patch.object(health.subprocess, "Popen", return_value=process), patch.object(health.os, "killpg") as kill, \
             self.assertRaisesRegex(health.GitCommandError, "DELIVERY_TIMEOUT") as raised:
            health.GitDelivery(self.root).git("ls-remote", health.REMOTE, health.REF)
        kill.assert_called_once_with(process.pid, health.signal.SIGKILL)
        self.assertEqual((raised.exception.step, raised.exception.failure), ("ls-remote", "TIMEOUT"))

    def test_git_failure_diagnostics_are_fixed_categories_not_private_stderr(self):
        cases = [
            (b"fatal: could not read Username for 'https://PRIVATE_MARKER': terminal prompts disabled", "AUTH_UNAVAILABLE"),
            (b"Authentication failed PRIVATE_MARKER", "AUTH_UNAVAILABLE"),
            (b"SSL certificate problem PRIVATE_MARKER", "TLS_ERROR"),
            (b"Recv failure: Connection reset by peer PRIVATE_MARKER", "NETWORK_ERROR"),
            (b"PRIVATE_MARKER non-fast-forward", "REF_CONFLICT"),
            (b"PRIVATE_MARKER remote rejected", "REMOTE_REJECTED"),
            (b"PRIVATE_MARKER unexpected failure", "UNKNOWN"),
        ]
        from unittest.mock import Mock
        for stderr, expected in cases:
            process = SimpleNamespace(returncode=128, communicate=Mock(return_value=(b"", stderr)))
            with self.subTest(expected=expected), patch.object(health.subprocess, "Popen", return_value=process), \
                 self.assertRaises(health.GitCommandError) as raised:
                health.GitDelivery(self.root).git("push", health.REMOTE, health.REF)
            self.assertEqual(str(raised.exception), "GIT_DELIVERY_FAILED")
            self.assertEqual((raised.exception.step, raised.exception.failure), ("push", expected))
            self.assertNotIn("PRIVATE_MARKER", str(vars(raised.exception)))
        unknown = health.GitCommandError("GIT_DELIVERY_FAILED", "PRIVATE_MARKER", "PRIVATE_MARKER")
        self.assertEqual((unknown.step, unknown.failure), ("OTHER", "UNKNOWN"))

    def test_failed_push_records_stage_without_mutating_receipt_or_success_time(self):
        path = health.health_dir(self.root) / "receipts/2026-10-03.json"
        original = receipt(code=3)
        original["attempts"][0]["sync"] = sync_evidence(
            "AVAILABLE", "INCOMPLETE", "YOJ_AUTH_REQUIRED", "2026-10-02T22:40:00+08:00")
        health.atomic_json(path, original)
        before = path.read_bytes()
        with patch.dict(os.environ, {"YOJ_SYNC_ALLOW_PUSH": "1"}), \
             patch.object(health, "now_local", return_value=at("2026-10-05T12:00:00+08:00")), \
             patch.object(health.GitDelivery, "publish", side_effect=health.GitCommandError(
                 "GIT_DELIVERY_FAILED", "push", "AUTH_UNAVAILABLE")), patch.object(health.time, "sleep"), \
             self.assertRaises(health.GitCommandError):
            health.deliver(self.root, replay=True)
        status = json.loads((health.health_dir(self.root) / "delivery-status.json").read_bytes())
        self.assertEqual(status, {"state": "DELIVERY_FAILED", "checkedAt": "2026-10-05T12:00:00+08:00",
                                  "category": "GIT_DELIVERY_FAILED", "gitStep": "push", "gitFailure": "AUTH_UNAVAILABLE"})
        self.assertEqual(path.read_bytes(), before)
        self.assertFalse((self.root / ".yoj-sync/launchd-cycle.json").exists())
        # Recovery clears stale delivery errors, but never rewrites business evidence.
        with patch.dict(os.environ, {"YOJ_SYNC_ALLOW_PUSH": "1"}), \
             patch.object(health, "now_local", return_value=at("2026-10-05T12:00:00+08:00")):
            sha = health.deliver(self.root, replay=True)
        self.assertEqual(path.read_bytes(), before)
        status = json.loads((health.health_dir(self.root) / "delivery-status.json").read_bytes())
        self.assertEqual(status["state"], "DELIVERED")
        self.assertNotIn("gitFailure", status)
        self.assertEqual(json.loads(self.git("--git-dir", str(self.remote), "show", f"{sha}:receipts/2026-10-03.json")), original)

    def test_replay_preserves_failed_journals_without_starting_a_new_cycle(self):
        path = health.health_dir(self.root) / "receipts/2026-10-03.json"
        original = receipt(code=3)
        original["attempts"][0]["sync"] = sync_evidence("BLOCKED", "BLOCKED", "YOJ_TLS_CERTIFICATE_ERROR",
                                                      "2026-10-02T22:40:00+08:00")
        health.atomic_json(path, original)
        before = path.read_bytes()
        with patch.dict(os.environ, {"YOJ_SYNC_ALLOW_PUSH": "1"}), \
             patch.object(health, "now_local", return_value=at("2026-10-05T12:00:00+08:00")):
            self.assertIsNone(health.deliver(self.root))  # automatic mode still gated
            sha = health.deliver(self.root, replay=True)
        self.assertEqual(path.read_bytes(), before)
        self.assertEqual(json.loads(self.git("--git-dir", str(self.remote), "show", f"{sha}:receipts/2026-10-03.json")), original)
        self.assertEqual(list(path.parent.glob("*.json")), [path])
        self.assertFalse((self.root / ".yoj-sync/launchd-cycle.json").exists())
        self.assertFalse((health.health_dir(self.root) / "contexts").exists())
        result = health.classify(original, original["cycleDate"], at("2026-10-05T12:00:00+08:00"))
        self.assertEqual((result["syncState"], result["lastSuccessfulSyncAt"]),
                         ("BLOCKED", "2026-10-02T22:40:00+08:00"))

    def test_wake_recovery_sends_only_authentic_pending_journal_once(self):
        path = health.health_dir(self.root) / "receipts/2026-10-03.json"
        original = receipt(code=3)
        original["attempts"][0]["sync"] = sync_evidence(
            "TRANSIENT_ERROR", "INCOMPLETE", "YOJ_TRANSIENT_NETWORK_ERROR", "2026-10-02T22:40:00+08:00")
        health.atomic_json(path, original)
        before = path.read_bytes()
        with patch.dict(os.environ, {"YOJ_SYNC_ALLOW_PUSH": "1"}), \
             patch.object(health, "now_local", return_value=at("2026-10-04T01:41:00+08:00")):
            self.assertIsNone(health.deliver(self.root))
            sha = health.deliver(self.root, pending_only=True)
            with patch.object(health.GitDelivery, "publish") as publish:
                self.assertIsNone(health.deliver(self.root, pending_only=True))
                publish.assert_not_called()
        self.assertEqual(path.read_bytes(), before)
        self.assertEqual(json.loads(self.git("--git-dir", str(self.remote), "show", f"{sha}:receipts/2026-10-03.json")), original)
        self.assertFalse((self.root / ".yoj-sync/launchd-cycle.json").exists())
        self.assertEqual(list(path.parent.glob("*.json")), [path])
        status = json.loads((health.health_dir(self.root) / "delivery-status.json").read_bytes())
        self.assertEqual(status["journalSha256"], health.journal_digest([original]))
        # A later terminal update still counts as pending after a RUNNING acknowledgement.
        running = receipt(outcome="RUNNING")
        health.atomic_json(path, running)
        health.atomic_json(health.health_dir(self.root) / "delivery-status.json", {
            "state": "DELIVERED", "journalSha256": health.journal_digest([running]),
            "checkedAt": "2026-10-03T22:31:00+08:00"})
        self.assertFalse(health.pending_delivery(self.root, at("2026-10-04T01:41:00+08:00")))
        health.atomic_json(path, original)
        self.assertTrue(health.pending_delivery(self.root, at("2026-10-04T01:41:00+08:00")))

    def test_wake_recovery_obeys_cooldown_maintenance_push_and_validation(self):
        path = health.health_dir(self.root) / "receipts/2026-10-03.json"
        health.atomic_json(path, receipt(code=3))
        health.atomic_json(health.health_dir(self.root) / "delivery-status.json", {
            "state": "DELIVERY_FAILED", "checkedAt": "2026-10-04T01:41:00+08:00",
            "category": "GIT_DELIVERY_FAILED", "gitStep": "ls-remote", "gitFailure": "NETWORK_ERROR"})
        for value, pending in [("01:55:59", False), ("01:56:00", True), ("00:05:00", False), ("23:55:00", False)]:
            self.assertEqual(health.pending_delivery(self.root, at(f"2026-10-04T{value}+08:00")), pending)
        with patch.object(health.GitDelivery, "publish") as publish, \
             patch.dict(os.environ, {"YOJ_SYNC_ALLOW_PUSH": "1"}), \
             patch.object(health, "now_local", return_value=at("2026-10-04T01:50:00+08:00")):
            self.assertIsNone(health.deliver(self.root, pending_only=True))
            publish.assert_not_called()
        with patch.dict(os.environ, {"YOJ_SYNC_ALLOW_PUSH": "0"}), self.assertRaises(health.HealthError):
            health.deliver(self.root, pending_only=True)
        invalid = receipt(code=3)
        invalid["private"] = "PRIVATE_MARKER"
        health.atomic_json(path, invalid)
        with patch.dict(os.environ, {"YOJ_SYNC_ALLOW_PUSH": "1"}), \
             patch.object(health, "now_local", return_value=at("2026-10-04T02:00:00+08:00")), \
             patch.object(health.GitDelivery, "publish") as publish, self.assertRaises(health.HealthError):
            health.deliver(self.root, pending_only=True)
        publish.assert_not_called()

    def test_empty_or_acknowledged_journal_does_not_launch_recovery_worker(self):
        with patch.dict(os.environ, {"YOJ_SYNC_ALLOW_PUSH": "1"}), \
             patch.object(health, "now_local", return_value=at("2026-10-04T01:41:00+08:00")), \
             patch.object(health.subprocess, "Popen") as child:
            health.spawn_delivery(self.root, pending_only=True)
            child.assert_not_called()
            health.atomic_json(health.health_dir(self.root) / "receipts/2026-10-03.json", receipt(code=3))
            health.spawn_delivery(self.root, pending_only=True)
            self.assertIn("retry-pending", child.call_args.args[0])
            self.assertNotIn("YOJ_LOGIN_PASS", child.call_args.kwargs["env"])
            child.reset_mock()
            health.atomic_json(health.health_dir(self.root) / "delivery-status.json", {
                "state": "DELIVERED", "journalSha256": health.journal_digest([receipt(code=3)])})
            health.spawn_delivery(self.root, pending_only=True)
            child.assert_not_called()

    def test_replay_cannot_bypass_push_gate_maintenance_or_validation(self):
        with patch.dict(os.environ, {"YOJ_SYNC_ALLOW_PUSH": "0"}), self.assertRaisesRegex(health.HealthError, "PUSH_DISABLED"):
            health.deliver(self.root, replay=True)
        with patch.dict(os.environ, {"YOJ_SYNC_ALLOW_PUSH": "1"}), self.assertRaisesRegex(health.HealthError, "OUTSIDE_DELIVERY_WINDOW"):
            health.deliver(self.root, replay=True)  # 23:56
        value = receipt()
        value["private"] = "PRIVATE_MARKER"
        health.atomic_json(health.health_dir(self.root) / "receipts/2026-10-03.json", value)
        with patch.dict(os.environ, {"YOJ_SYNC_ALLOW_PUSH": "1"}), \
             patch.object(health, "now_local", return_value=at("2026-10-05T12:00:00+08:00")), \
             patch.object(health.GitDelivery, "publish") as publish, self.assertRaises(health.HealthError):
            health.deliver(self.root, replay=True)
        publish.assert_not_called()

    def test_transient_failures_retry_fresh_snapshot_and_status_is_private_safe(self):
        health.atomic_json(health.health_dir(self.root) / "receipts/2026-10-03.json", receipt(code=3))
        for errors, expected, calls in [
            ([health.HealthError("GIT_DELIVERY_FAILED"), "a" * 40], "DELIVERED", 2),
            ([health.HealthError("DELIVERY_TIMEOUT")] * 3, "DELIVERY_FAILED", 3),
            ([health.HealthError("RECEIPT_CONFLICT")], "DELIVERY_FAILED", 1),
            ([OSError("PRIVATE_MARKER")], "DELIVERY_FAILED", 1),
        ]:
            with self.subTest(expected=expected, calls=calls), patch.dict(os.environ, {"YOJ_SYNC_ALLOW_PUSH": "1"}), \
                 patch.object(health, "now_local", return_value=at("2026-10-05T12:00:00+08:00")), \
                 patch.object(health.GitDelivery, "publish", side_effect=errors) as publish, patch.object(health.time, "sleep"):
                if expected == "DELIVERED":
                    health.deliver(self.root, replay=True)
                else:
                    with self.assertRaises((OSError, health.HealthError)):
                        health.deliver(self.root, replay=True)
            self.assertEqual(publish.call_count, calls)
            status = json.loads((health.health_dir(self.root) / "delivery-status.json").read_bytes())
            self.assertEqual(status["state"], expected)
            self.assertNotIn("PRIVATE_MARKER", str(status))

    def test_transient_retries_stop_when_total_budget_is_exhausted(self):
        def exhausted(publisher, *args):
            publisher.deadline = health.time.monotonic()
            raise health.HealthError("DELIVERY_TIMEOUT")
        with patch.dict(os.environ, {"YOJ_SYNC_ALLOW_PUSH": "1"}), \
             patch.object(health, "now_local", return_value=at("2026-10-05T12:00:00+08:00")), \
             patch.object(health.GitDelivery, "publish", autospec=True, side_effect=exhausted) as publish, \
             patch.object(health.time, "sleep") as sleep, self.assertRaises(health.HealthError):
            health.deliver(self.root, replay=True)
        self.assertEqual(publish.call_count, 1)
        sleep.assert_not_called()


class PublicCheckerTests(unittest.TestCase):
    def item(self, payload):
        raw = health.encode(payload)
        return {"type": "file", "encoding": "base64", "size": len(raw), "content": base64.b64encode(raw).decode()}

    def test_receipts_are_read_at_one_resolved_sha_and_missing_day_visible(self):
        sha = "a" * 40
        with patch.object(health, "public_json", side_effect=[{"ref": health.REF, "object": {"sha": sha}}, self.item(receipt()), None]) as get:
            results = health.check_cycles(["2026-10-03", "2026-10-02"], at())
        self.assertEqual([r["scheduleState"] for r in results], ["ON_TIME", "NO_RECEIPT"])
        self.assertTrue(all(f"ref={sha}" in call.args[0] for call in get.call_args_list[1:]))

    def test_missing_branch_is_not_the_same_as_unreadable_repository(self):
        with patch.object(health, "public_json", side_effect=[None, {"id": 1}]):
            self.assertEqual(health.check_cycles(["2026-10-03"], at())[0]["scheduleState"], "NO_RECEIPT")
        with patch.object(health, "public_json", side_effect=[None, None]), self.assertRaises(health.HealthError):
            health.check_cycles(["2026-10-03"], at())

    def test_invalid_public_fields_are_never_echoed(self):
        value = receipt()
        value["private"] = "PRIVATE_MARKER"
        with patch.object(health, "public_json", side_effect=[{"ref": health.REF, "object": {"sha": "a" * 40}}, self.item(value)]):
            results = health.check_cycles(["2026-10-03"], at())
        self.assertEqual(results[0]["scheduleState"], "INVALID_RECEIPT")
        self.assertNotIn("PRIVATE_MARKER", health.render_summary(results))

    def test_network_failure_is_bounded_and_not_missing_receipt(self):
        with patch.object(health.urllib.request, "urlopen", side_effect=OSError("PRIVATE_MARKER")) as request, \
             patch.object(health.subprocess, "run", side_effect=OSError("PRIVATE_MARKER")) as fallback, \
             patch.object(health.time, "sleep"), self.assertRaisesRegex(health.HealthError, "OBSERVATION_ERROR"):
            health.public_json("git/ref/heads/example")
        self.assertEqual(request.call_count + fallback.call_count, 3)
        self.assertFalse(request.call_args.args[0].has_header("Authorization"))
        for status in (403, 429, 500):
            error = urllib.error.HTTPError("https://example.invalid", status, "PRIVATE_MARKER", {}, None)
            with self.subTest(status=status), patch.object(health.urllib.request, "urlopen", side_effect=error), \
                 patch.object(health.time, "sleep"), self.assertRaises(health.HealthError):
                health.public_json("git/ref/heads/example")

    def test_system_tls_fallback_is_anonymous_verified_and_size_limited(self):
        response = SimpleNamespace(returncode=0, stdout=b'{"ref":"safe"}\n200')
        with patch.object(health.urllib.request, "urlopen", side_effect=OSError("certificate roots unavailable")), \
             patch.object(health.subprocess, "run", return_value=response) as curl, patch.object(health.time, "sleep"):
            self.assertEqual(health.public_json("git/ref/heads/example"), {"ref": "safe"})
        command = curl.call_args.args[0]
        self.assertEqual(command[:2], ["curl", "--disable"])
        self.assertNotIn("--insecure", command)
        self.assertNotIn("-k", command)
        self.assertNotIn("Authorization", " ".join(command))
        self.assertIn("--max-filesize", command)

    def test_http_error_response_is_closed_on_404_and_repeated_retries(self):
        import io
        for status, calls in [(404, 1), (403, 3)]:
            with self.subTest(status=status):
                stream = io.BytesIO(b"fixture response")
                error = urllib.error.HTTPError("https://example.invalid", status, "PRIVATE_MARKER", {}, stream)
                with patch.object(health.urllib.request, "urlopen", side_effect=error) as request, \
                     patch.object(health.time, "sleep"):
                    if status == 404:
                        self.assertIsNone(health.public_json("git/ref/heads/example"))
                    else:
                        with self.assertRaisesRegex(health.HealthError, "OBSERVATION_ERROR"):
                            health.public_json("git/ref/heads/example")
                self.assertEqual(request.call_count, calls)
                self.assertTrue(stream.closed)

    def test_system_tls_http_error_is_not_a_receipt(self):
        for status, missing in [(b"404", True), (b"403", False), (b"503", False)]:
            with self.subTest(status=status), patch.object(health.urllib.request, "urlopen", side_effect=OSError("TLS roots")), \
                 patch.object(health.subprocess, "run", return_value=SimpleNamespace(returncode=0, stdout=b'{}\n' + status)), \
                 patch.object(health.time, "sleep"):
                if missing:
                    self.assertIsNone(health.public_json("git/ref/heads/example"))
                else:
                    with self.assertRaisesRegex(health.HealthError, "OBSERVATION_ERROR"):
                        health.public_json("git/ref/heads/example")

    def test_cli_exit_distinguishes_scheduling_from_business_failure(self):
        import io
        for payload, expected in [(receipt(code=8), 0), (receipt(outcome="RUNNING"), 0)]:
            result = health.classify(payload, "2026-10-03", at())
            with patch.object(health, "now_local", return_value=at()), patch.object(health, "check_cycles", return_value=[result]), \
                 patch.object(health.sys, "argv", ["yoj_health.py", "check"]), patch("sys.stdout", new_callable=io.StringIO) as output:
                self.assertEqual(health.main(), expected)
                self.assertIn("::warning::", output.getvalue())
        with patch.object(health, "now_local", return_value=at()), \
             patch.object(health, "check_cycles", return_value=[{"cycleDate": "2026-10-03", "scheduleState": "NO_RECEIPT", "runState": "UNKNOWN"}]), \
             patch.object(health.sys, "argv", ["yoj_health.py", "check"]), patch("sys.stdout", new_callable=io.StringIO) as output:
            self.assertEqual(health.main(), 1)
            self.assertIn("::error::2026-10-03: scheduling=NO_RECEIPT", output.getvalue())

    def test_optional_sync_requirement_and_summary_do_not_change_default_schedule_check(self):
        import io
        succeeded, blocked, legacy = receipt(), receipt(code=3), receipt()
        succeeded["attempts"][0]["sync"] = sync_evidence()
        blocked["attempts"][0]["sync"] = sync_evidence("BLOCKED", "BLOCKED", "YOJ_TLS_CERTIFICATE_ERROR",
                                                      "2026-10-02T22:40:00+08:00")
        for payload, required, expected in [(legacy, False, 0), (legacy, True, 1), (succeeded, True, 0),
                                             (blocked, False, 0), (blocked, True, 1)]:
            with self.subTest(required=required, sync=payload["attempts"][0].get("sync")):
                result = health.classify(payload, "2026-10-03", at())
                argv = ["yoj_health.py", "check"] + (["--require-sync"] if required else [])
                with patch.object(health, "now_local", return_value=at()), patch.object(health, "check_cycles", return_value=[result]), \
                     patch.object(health.sys, "argv", argv), patch("sys.stdout", new_callable=io.StringIO) as output:
                    self.assertEqual(health.main(), expected)
                rendered = output.getvalue()
                self.assertIn("| Scheduling | Execution | Source | Sync | Reason | Last successful sync", rendered)
                if payload is legacy:
                    self.assertIn("sync=UNKNOWN", rendered)
                if payload is blocked:
                    self.assertIn("source=BLOCKED; sync=BLOCKED; reason=YOJ_TLS_CERTIFICATE_ERROR", rendered)
                    self.assertIn("2026-10-02T22:40:00+08:00", rendered)


if __name__ == "__main__":
    unittest.main()
