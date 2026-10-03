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


class ReceiptTests(unittest.TestCase):
    def test_no_change_and_business_failure_both_prove_scheduled_execution(self):
        for code, run in [(0, "RETURNED_OK"), (3, "RETURNED_NONZERO"), (4, "RETURNED_NONZERO"), (8, "RETURNED_NONZERO")]:
            with self.subTest(code=code):
                result = health.classify(receipt(code=code), "2026-10-03", at())
                self.assertEqual((result["scheduleState"], result["runState"]), ("ON_TIME", run))

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
             self.assertRaisesRegex(health.HealthError, "DELIVERY_TIMEOUT"):
            health.GitDelivery(self.root).git("ls-remote", health.REMOTE, health.REF)
        kill.assert_called_once_with(process.pid, health.signal.SIGKILL)


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
             patch.object(health.time, "sleep"), self.assertRaisesRegex(health.HealthError, "OBSERVATION_ERROR"):
            health.public_json("git/ref/heads/example")
        self.assertEqual(request.call_count, 3)
        self.assertFalse(request.call_args.args[0].has_header("Authorization"))
        for status in (403, 429, 500):
            error = urllib.error.HTTPError("https://example.invalid", status, "PRIVATE_MARKER", {}, None)
            with self.subTest(status=status), patch.object(health.urllib.request, "urlopen", side_effect=error), \
                 patch.object(health.time, "sleep"), self.assertRaises(health.HealthError):
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
             patch.object(health.sys, "argv", ["yoj_health.py", "check"]), patch("sys.stdout", new_callable=io.StringIO):
            self.assertEqual(health.main(), 1)


if __name__ == "__main__":
    unittest.main()
