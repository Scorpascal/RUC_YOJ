"""Offline evidence checks; subprocess/network doubles do not validate real TLS."""

from __future__ import annotations

import argparse
import base64
import contextlib
import copy
import io
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from tools import ubuntu_compat_receipts as probes, yoj_health as health


CANDIDATE = "1" * 40
RECEIPT_SHA = "2" * 40
BLOB_SHA = "3" * 40
CYCLE = "2026-10-03"
OBSERVED_AT = "2026-10-05T00:00:00+08:00"


def receipt(cycle: str = CYCLE, code: int = 0, *, on_time: bool = True,
            successful_sync: bool = True) -> dict:
    entered = "22:30:00" if on_time else "23:30:00"
    attempt = {"attemptNo": 1, "invokedAt": f"{cycle}T{entered}+08:00",
               "enteredAt": f"{cycle}T{entered}+08:00",
               "finishedAt": f"{cycle}T23:40:00+08:00", "outcome": "RETURNED",
               "exitCode": code, "skipReason": None}
    if successful_sync:
        attempt["sync"] = {"sourceState": "AVAILABLE", "syncState": "SUCCEEDED",
                           "reason": "NONE", "lastSuccessfulSyncAt": attempt["finishedAt"]}
    elif code:
        attempt["sync"] = {"sourceState": "ERROR", "syncState": "FAILED",
                           "reason": "SYNC_FAILED", "lastSuccessfulSyncAt": None}
    return {"schemaVersion": 1, "cycleDate": cycle, "attempts": [attempt]}


def contents(payload: dict) -> dict:
    raw = health.encode(payload)
    return {"type": "file", "encoding": "base64", "size": len(raw),
            "content": base64.b64encode(raw).decode("ascii"), "sha": BLOB_SHA}


def cli_process(summary: str, code: int):
    """Replace only execution: online_cli still reads and validates its output."""
    def run(command, **kwargs):
        Path(command[command.index("--summary") + 1]).write_text(summary, encoding="utf-8")
        return subprocess.CompletedProcess(command, code, stdout="", stderr="")
    return run


class ReceiptObservationTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.snapshot = self.root / "snapshot"
        self.snapshot.mkdir()
        self.output = self.root / "output"
        self.output.mkdir()
        self.ref = {"ref": health.REF, "object": {"sha": RECEIPT_SHA}}
        self.env = patch.dict(os.environ, {"GITHUB_SHA": CANDIDATE, "GITHUB_OUTPUT": "",
                                          "GITHUB_STEP_SUMMARY": ""})
        self.env.start()
        self.addCleanup(self.env.stop)

    def fixed_snapshot(self, payload: dict) -> dict:
        metadata = {"schemaVersion": 1, "candidateSha": CANDIDATE,
                    "cycleDate": payload["cycleDate"], "observedAt": OBSERVED_AT,
                    "receiptSha": RECEIPT_SHA, "receiptBlobSha": BLOB_SHA}
        item = contents(payload)
        metadata["expected"] = probes.snapshot_cli(
            metadata, self.ref, item, self.snapshot / "expected-summary.md")
        for name, value in (("metadata", metadata), ("ref", self.ref), ("contents", item)):
            probes.write_json(self.snapshot / f"{name}.json", value)
        return metadata

    def online(self, payload: dict, *, code: int | None = None) -> dict:
        expected = self.fixed_snapshot(payload)["expected"]
        with patch.object(probes.subprocess, "run", side_effect=cli_process(
                expected["summary"], expected["exitCode"] if code is None else code)):
            return probes.online_cli(payload["cycleDate"], self.output)

    def observe(self, payload: dict, *, online_summary: str | None = None,
                online_code: int | None = None) -> tuple[int, dict, str]:
        expected = self.fixed_snapshot(payload)["expected"]
        online_summary = expected["summary"] if online_summary is None else online_summary
        online_code = expected["exitCode"] if online_code is None else online_code
        with patch.object(probes.subprocess, "run", side_effect=cli_process(online_summary, online_code)), \
                patch.object(health, "public_json", return_value=self.ref), \
                patch.object(probes, "native_https", return_value={"status": "PASS", "sha": RECEIPT_SHA}), \
                patch.object(probes, "curl_fallback", return_value={"status": "PASS", "sha": RECEIPT_SHA, "curlCalls": 1}), \
                contextlib.redirect_stdout(io.StringIO()):
            code = probes.observe(argparse.Namespace(snapshot=self.snapshot, output=self.output,
                                                       runner="ubuntu-26.04"))
        return (code, json.loads((self.output / "result.json").read_text(encoding="utf-8")),
                (self.output / "report.md").read_text(encoding="utf-8"))

    def test_select_prefers_healthy_completed_receipt_over_newer_business_failure(self):
        healthy, unhealthy = receipt(), receipt("2026-10-04", 8, successful_sync=False)
        replies = {probes.REF_ENDPOINT: self.ref,
                   f"git/trees/{RECEIPT_SHA}?recursive=1": {"tree": [
                       {"type": "blob", "path": f"receipts/{item['cycleDate']}.json"}
                       for item in (healthy, unhealthy)]},
                   **{f"contents/receipts/{item['cycleDate']}.json?ref={RECEIPT_SHA}": contents(item)
                      for item in (healthy, unhealthy)}}
        with patch.object(health, "now_local", return_value=health.timestamp(OBSERVED_AT)), \
                patch.object(health, "public_json", side_effect=replies.__getitem__), \
                contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(probes.select(argparse.Namespace(snapshot=self.snapshot)), 0)
        selected = json.loads((self.snapshot / "metadata.json").read_text(encoding="utf-8"))
        self.assertEqual(selected["cycleDate"], CYCLE)
        self.assertEqual(selected["expected"]["exitCode"], 0)
        self.assertEqual(selected["expected"]["results"][0]["syncState"], "SUCCEEDED")

    def test_ref_sha_rejects_nonobject_payload_with_public_error_category(self):
        for value in ([], None):
            with self.subTest(object=value), self.assertRaisesRegex(
                    health.HealthError, "^INVALID_REMOTE_REF$"):
                probes.ref_sha({"ref": health.REF, "object": value})

    def test_select_preserves_valid_completed_business_failure_when_only_evidence(self):
        payload = receipt(code=8, successful_sync=False)
        replies = {probes.REF_ENDPOINT: self.ref,
                   f"git/trees/{RECEIPT_SHA}?recursive=1": {"tree": [
                       {"type": "blob", "path": f"receipts/{CYCLE}.json"}]},
                   f"contents/receipts/{CYCLE}.json?ref={RECEIPT_SHA}": contents(payload)}
        with patch.object(health, "now_local", return_value=health.timestamp(OBSERVED_AT)), \
                patch.object(health, "public_json", side_effect=replies.__getitem__), \
                contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(probes.select(argparse.Namespace(snapshot=self.snapshot)), 0)
        selected = json.loads((self.snapshot / "metadata.json").read_text(encoding="utf-8"))
        self.assertEqual(selected["expected"]["exitCode"], 0)
        state = selected["expected"]["results"][0]
        self.assertEqual((state["scheduleState"], state["runState"], state["syncState"]),
                         ("ON_TIME", "RETURNED_NONZERO", "FAILED"))
        self.assertEqual(state["attempts"][0]["exitCode"], 8)

    def test_online_on_time_business_failure_is_valid_observation_with_original_exit_zero(self):
        result = self.online(receipt(code=8, successful_sync=False))
        self.assertEqual((result["status"], result["exitCode"]), ("PASS", 0))
        self.assertEqual(result["businessHealth"], "UNHEALTHY")
        self.assertEqual((result["states"]["scheduleState"], result["states"]["runState"]),
                         ("ON_TIME", "RETURNED_NONZERO"))
        self.assertIn("RETURNED, exit: 8", result["summary"])

    def test_online_out_of_window_preserves_original_nonzero_as_valid_observation(self):
        result = self.online(receipt(on_time=False))
        self.assertEqual((result["status"], result["exitCode"]), ("PASS", 1))
        self.assertEqual(result["businessHealth"], "UNHEALTHY")
        self.assertEqual(result["states"]["scheduleState"], "OUT_OF_WINDOW")

    def test_online_distinguishes_explicit_success_from_legacy_unknown_sync(self):
        for successful, expected in ((True, "HEALTHY"), (False, "UNKNOWN")):
            with self.subTest(successful_sync=successful):
                result = self.online(receipt(successful_sync=successful))
                self.assertEqual((result["status"], result["exitCode"]), ("PASS", 0))
                self.assertEqual(result["businessHealth"], expected)

    def test_online_network_failure_and_missing_summary_are_unverified(self):
        for failure in (OSError("offline"), subprocess.TimeoutExpired("check", 100)):
            with self.subTest(failure=type(failure).__name__), \
                    patch.object(probes.subprocess, "run", side_effect=failure):
                result = probes.online_cli(CYCLE, self.output)
                self.assertEqual((result["status"], result["exitCode"]), ("FAIL", None))
                self.assertEqual(result["states"], {})
                self.assertEqual(result["businessHealth"], "UNKNOWN")
        with patch.object(probes.subprocess, "run", return_value=subprocess.CompletedProcess([], 0)):
            result = probes.online_cli(CYCLE, self.output)
        self.assertEqual(result["status"], "FAIL")
        self.assertEqual(result["summary"], "")
        self.assertEqual(result["states"], {})

    def test_online_does_not_reuse_a_previous_summary_when_current_cli_writes_none(self):
        classified = health.classify(receipt(), CYCLE, health.timestamp(OBSERVED_AT))
        (self.output / "online-summary.md").write_text(health.render_summary([classified]), encoding="utf-8")
        with patch.object(probes.subprocess, "run", return_value=subprocess.CompletedProcess([], 0)):
            result = probes.online_cli(CYCLE, self.output)
        self.assertEqual(result["status"], "FAIL")
        self.assertEqual(result["summary"], "")
        self.assertEqual(result["exitCode"], 0)

    def test_online_error_empty_or_invalid_receipt_summary_never_provides_valid_observation(self):
        cases = [(health.render_summary([], "OBSERVATION_ERROR"), 1),
                 (health.render_summary([]), 0),
                 (health.render_summary([{"cycleDate": CYCLE, "scheduleState": "INVALID_RECEIPT",
                                          "runState": "UNKNOWN"}]), 1),
                 (health.render_summary([{"cycleDate": CYCLE, "scheduleState": "NO_RECEIPT",
                                          "runState": "UNKNOWN"}]), 1)]
        for summary, code in cases:
            with self.subTest(summary=summary), patch.object(
                    probes.subprocess, "run", side_effect=cli_process(summary, code)):
                result = probes.online_cli(CYCLE, self.output)
                self.assertEqual(result["status"], "FAIL")
                self.assertEqual(result["exitCode"], code)
                self.assertEqual(result["businessHealth"], "UNKNOWN")

    def test_online_illegal_states_and_exit_semantics_do_not_manufacture_pass(self):
        classified = health.classify(receipt(), CYCLE, health.timestamp(OBSERVED_AT))
        cases = []
        for field in ("scheduleState", "runState", "sourceState", "syncState", "reason"):
            changed = copy.deepcopy(classified)
            changed[field] = "NOT_A_STATE"
            cases.append((health.render_summary([changed]), 0))
        summary = health.render_summary([classified])
        cases.extend(((summary, 1), (health.render_summary([classified, classified]), 0)))
        out_of_window = health.classify(receipt(on_time=False), CYCLE, health.timestamp(OBSERVED_AT))
        cases.append((health.render_summary([out_of_window]), 0))
        for summary, code in cases:
            with self.subTest(summary=summary, code=code), patch.object(
                    probes.subprocess, "run", side_effect=cli_process(summary, code)):
                result = probes.online_cli(CYCLE, self.output)
                self.assertEqual(result["status"], "FAIL")
                self.assertEqual(result["exitCode"], code)
                self.assertEqual(result["businessHealth"], "UNKNOWN")

    def test_observe_keeps_business_failure_separate_from_compatibility_and_original_exit(self):
        for payload, original_exit in ((receipt(code=8, successful_sync=False), 0),
                                       (receipt(on_time=False), 1)):
            with self.subTest(original_exit=original_exit):
                code, result, report = self.observe(payload)
                self.assertEqual((code, result["status"]), (0, "PASS"))
                self.assertEqual(result["onlineCli"]["businessHealth"], "UNHEALTHY")
                self.assertEqual(result["snapshotCli"]["exitCode"], original_exit)
                self.assertEqual(result["onlineCli"]["exitCode"], original_exit)
                self.assertIn("UNHEALTHY", report)
                self.assertIn("historical nonzero business exit remains visible", report)
                self.assertIn("Overall receipt/TLS probes: **PASS**", report)

    def test_observe_missing_online_evidence_fails_even_when_fixed_snapshot_and_probes_pass(self):
        code, result, report = self.observe(receipt(),
                                           online_summary=health.render_summary([], "OBSERVATION_ERROR"),
                                           online_code=1)
        self.assertEqual((code, result["status"]), (1, "FAIL"))
        self.assertEqual(result["snapshotCli"]["status"], "PASS")
        self.assertEqual(result["onlineCli"]["status"], "FAIL")
        self.assertEqual(result["onlineCli"]["exitCode"], 1)
        self.assertIn("Check unavailable", report)
        self.assertIn("Overall receipt/TLS probes: **FAIL**", report)


if __name__ == "__main__":
    unittest.main()
