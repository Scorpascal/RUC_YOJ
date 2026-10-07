"""Offline evidence and failure-gate tests for the cross-runner receipt comparison."""

from __future__ import annotations

import contextlib
import copy
import io
import json
import os
import sys
import tempfile
import unittest
from datetime import datetime
from pathlib import Path
from unittest.mock import patch

from tools import yoj_health as health

with patch.dict(sys.modules, {"yoj_health": health}):
    from tools import ubuntu_compat_receipts as compat


CYCLE = "2026-10-03"
OBSERVED_AT = "2026-10-04T00:00:00+08:00"
CANDIDATE_SHA = "a" * 40
RECEIPT_SHA = "b" * 40
BLOB_SHA = "c" * 40


def observation(runner: str, *, late: bool = False, business_exit: int = 0) -> dict:
    """Build one small, valid receipt using the production classifier and summary."""
    entered = "23:40:00" if late else "22:30:00"
    payload = {"schemaVersion": 1, "cycleDate": CYCLE, "attempts": [{
        "attemptNo": 1, "invokedAt": f"{CYCLE}T{entered}+08:00",
        "enteredAt": f"{CYCLE}T{entered}+08:00", "finishedAt": f"{CYCLE}T23:45:00+08:00",
        "outcome": "RETURNED", "exitCode": business_exit, "skipReason": None,
    }]}
    classified = health.classify(payload, CYCLE, datetime.fromisoformat(OBSERVED_AT))
    summary = health.render_summary([classified])
    states = {key: classified[key] for key in (
        "scheduleState", "runState", "sourceState", "syncState", "reason")}
    states["lastSuccessfulSyncAt"] = classified["lastSuccessfulSyncAt"] or "unknown"
    code = int(classified["scheduleState"] != "ON_TIME")
    return {
        "schemaVersion": 1, "runner": runner, "candidateSha": CANDIDATE_SHA,
        "receiptSha": RECEIPT_SHA, "receiptBlobSha": BLOB_SHA, "cycleDate": CYCLE,
        "snapshotObservedAt": OBSERVED_AT,
        "onlineRefBefore": {"status": "PASS", "sha": RECEIPT_SHA},
        "onlineRefAfter": {"status": "PASS", "sha": RECEIPT_SHA},
        "onlineRefStatus": "STABLE",
        "onlineCli": {"status": "PASS", "exitCode": code, "states": states,
                      "summary": summary, "error": None},
        "snapshotCli": {"status": "PASS", "exitCode": code, "results": [classified],
                        "summary": summary},
        "nativeHttps": {"status": "PASS", "sha": RECEIPT_SHA},
        "curlFallback": {"status": "PASS", "sha": RECEIPT_SHA, "curlCalls": 1},
        "status": "PASS",
    }


class ReceiptComparisonTests(unittest.TestCase):
    def compare(self, values: list[dict | str]) -> tuple[int, dict, str]:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for index, value in enumerate(values):
                path = root / "results" / str(index) / "result.json"
                path.parent.mkdir(parents=True)
                path.write_text(value if isinstance(value, str) else json.dumps(value), encoding="utf-8")
            target = root / "comparison"
            argv = ["ubuntu_compat_receipts.py", "compare", "--results", str(root / "results"),
                    "--output", str(target)]
            with patch.object(sys, "argv", argv), patch.dict(os.environ, {"GITHUB_STEP_SUMMARY": ""}), \
                    patch.object(health, "public_json", side_effect=AssertionError("offline test")), \
                    contextlib.redirect_stdout(io.StringIO()):
                code = compat.main()
            self.assertTrue((target / "comparison.json").is_file(), "Every result, including invalid input, needs a report")
            self.assertTrue((target / "comparison.md").is_file(), "Every result needs a readable conclusion")
            return code, json.loads((target / "comparison.json").read_text(encoding="utf-8")), \
                (target / "comparison.md").read_text(encoding="utf-8")

    def pair(self, **kwargs) -> list[dict]:
        return [observation(runner, **kwargs) for runner in sorted(compat.RUNNERS)]

    def assert_failed(self, values: list[dict | str]) -> dict:
        code, result, report = self.compare(values)
        self.assertNotEqual(code, 0)
        self.assertEqual(result["status"], "FAIL")
        self.assertIn("Overall: **FAIL**", report)
        self.assertNotIn("Overall: **PASS**", report)
        return result

    def assert_snapshot_pass(self, values: list[dict], *, online_comparable: bool = True) -> tuple[dict, str]:
        code, result, report = self.compare(values)
        self.assertEqual(code, 0)
        self.assertEqual(result["status"], "PASS")
        self.assertIs(result["sameCandidateAndSnapshot"], True)
        self.assertIs(result["snapshotStatesSummaryAndExitMatch"], True)
        self.assertIs(result["onlineComparable"], online_comparable)
        self.assertIn("Overall: **PASS**", report)
        self.assertIn(f"Online branch snapshots directly comparable: **{online_comparable}**", report)
        return result, report

    def test_matching_valid_evidence_passes_the_gate(self):
        result, report = self.assert_snapshot_pass(self.pair())
        self.assertIs(result["onlineStatesSummaryAndExitMatch"], True)
        self.assertIn("online states/summary/exit match: **True**", report)

    def test_missing_duplicate_or_extra_runner_results_fail(self):
        pair = self.pair()
        for name, values in (("missing", pair[:1]), ("duplicate", [pair[0], copy.deepcopy(pair[0])]),
                             ("extra", pair + [copy.deepcopy(pair[0])]), ("empty", [])):
            with self.subTest(case=name):
                result = self.assert_failed(values)
                self.assertIs(result["sameCandidateAndSnapshot"], False)

    def test_candidate_or_snapshot_identity_mismatch_fails(self):
        replacements = {"candidateSha": "d" * 40, "receiptSha": "d" * 40,
                        "receiptBlobSha": "d" * 40, "cycleDate": "2026-10-04",
                        "snapshotObservedAt": "2026-10-04T00:01:00+08:00"}
        for key, value in replacements.items():
            with self.subTest(field=key):
                pair = self.pair()
                pair[1][key] = value
                result = self.assert_failed(pair)
                self.assertIs(result["sameCandidateAndSnapshot"], False)

    def test_fixed_snapshot_states_summary_or_exit_difference_fails(self):
        for field in ("results", "summary", "exitCode"):
            with self.subTest(field=field):
                pair = self.pair()
                replacement = observation(pair[1]["runner"], late=True)["snapshotCli"]
                pair[1]["snapshotCli"][field] = replacement[field]
                result = self.assert_failed(pair)
                self.assertIs(result["snapshotStatesSummaryAndExitMatch"], False)

    def test_any_runner_overall_failure_fails(self):
        for index in range(2):
            with self.subTest(runner=index):
                pair = self.pair()
                pair[index]["status"] = "FAIL"
                self.assert_failed(pair)

    def test_top_level_pass_cannot_hide_failed_subprobes(self):
        for key in ("nativeHttps", "curlFallback", "snapshotCli", "onlineCli"):
            for indices in ((0,), (0, 1)):
                with self.subTest(probe=key, failed_runners=indices):
                    pair = self.pair()
                    for index in indices:
                        pair[index][key]["status"] = "FAIL"
                    self.assert_failed(pair)

    def test_internally_inconsistent_online_states_summary_or_exit_fails(self):
        for field in ("states", "summary", "exitCode"):
            with self.subTest(field=field):
                pair = self.pair()
                replacement = observation(pair[1]["runner"], late=True)["onlineCli"]
                pair[1]["onlineCli"][field] = replacement[field]
                result = self.assert_failed(pair)
                self.assertIsNot(result["onlineStatesSummaryAndExitMatch"], True)

    def test_valid_online_semantic_difference_at_the_same_stable_ref_fails(self):
        pair = self.pair()
        pair[1]["onlineCli"] = observation(pair[1]["runner"], late=True)["onlineCli"]
        result = self.assert_failed(pair)
        self.assertIs(result["onlineComparable"], True)
        self.assertIs(result["onlineStatesSummaryAndExitMatch"], False)

    def test_moving_online_ref_uses_snapshot_without_claiming_online_equality(self):
        pair = self.pair()
        pair[0]["onlineRefAfter"]["sha"] = "d" * 40
        pair[0]["onlineRefStatus"] = "CHANGED"
        pair[0]["onlineCli"] = observation(pair[0]["runner"], late=True)["onlineCli"]
        result, report = self.assert_snapshot_pass(pair, online_comparable=False)
        self.assertIsNone(result["onlineStatesSummaryAndExitMatch"])
        self.assertIn("online results are observations only", report)
        self.assertNotIn("online states/summary/exit match: **True**", report)

    def test_different_online_shas_use_snapshot_without_claiming_online_equality(self):
        pair = self.pair()
        pair[1]["onlineRefBefore"]["sha"] = "d" * 40
        pair[1]["onlineRefAfter"]["sha"] = "d" * 40
        pair[1]["onlineCli"] = observation(pair[1]["runner"], business_exit=3)["onlineCli"]
        result, report = self.assert_snapshot_pass(pair, online_comparable=False)
        self.assertIsNone(result["onlineStatesSummaryAndExitMatch"])
        self.assertIn("online results are observations only", report)
        self.assertNotIn("online states/summary/exit match: **True**", report)

    def test_matching_nonzero_schedule_exit_is_compatible_but_unhealthy(self):
        result, report = self.assert_snapshot_pass(self.pair(late=True))
        self.assertIs(result["onlineStatesSummaryAndExitMatch"], True)
        self.assertIn("1 / PASS", report)
        self.assertIn("OUT_OF_WINDOW", report)
        self.assertIn("UNHEALTHY", report)
        self.assertEqual([item["snapshotCli"]["exitCode"] for item in result["results"]], [1, 1])

    def test_damaged_json_has_an_explicit_failure_report(self):
        self.assert_failed([self.pair()[0], '{"runner":'])

    def test_missing_required_evidence_fields_fail_even_when_both_match(self):
        paths = [(key,) for key in self.pair()[0]] + [
            ("snapshotCli", "status"), ("snapshotCli", "exitCode"), ("snapshotCli", "results"),
            ("snapshotCli", "summary"), ("onlineCli", "status"), ("onlineCli", "exitCode"),
            ("onlineCli", "states"), ("onlineCli", "summary"),
            ("onlineRefBefore", "status"), ("onlineRefBefore", "sha"),
            ("onlineRefAfter", "status"), ("onlineRefAfter", "sha"),
            ("nativeHttps", "status"), ("nativeHttps", "sha"),
            ("curlFallback", "status"), ("curlFallback", "sha"), ("curlFallback", "curlCalls"),
            ("onlineCli", "states", "scheduleState"), ("onlineCli", "states", "runState"),
            ("onlineCli", "states", "sourceState"), ("onlineCli", "states", "syncState"),
            ("onlineCli", "states", "reason"), ("onlineCli", "states", "lastSuccessfulSyncAt"),
        ]
        for path in paths:
            with self.subTest(field=".".join(path)):
                pair = self.pair()
                for value in pair:
                    for key in path[:-1]:
                        value = value[key]
                    del value[path[-1]]
                self.assert_failed(pair)

    def test_illegal_status_and_state_values_fail_even_when_both_match(self):
        paths = [("status",), ("onlineRefStatus",),
                 ("onlineRefBefore", "status"), ("onlineRefAfter", "status"),
                 ("snapshotCli", "status"), ("onlineCli", "status"),
                 ("nativeHttps", "status"), ("curlFallback", "status"),
                 ("onlineCli", "states", "scheduleState"), ("onlineCli", "states", "runState"),
                 ("onlineCli", "states", "sourceState"), ("onlineCli", "states", "syncState"),
                 ("onlineCli", "states", "reason")]
        for path in paths:
            with self.subTest(field=".".join(path)):
                pair = self.pair()
                for value in pair:
                    for key in path[:-1]:
                        value = value[key]
                    value[path[-1]] = "NOT_A_REAL_STATE"
                self.assert_failed(pair)
        pair = self.pair()
        for value in pair:
            value["snapshotCli"]["results"][0]["scheduleState"] = "NOT_A_REAL_STATE"
        self.assert_failed(pair)

    def test_invalid_identity_and_exit_types_cannot_manufacture_matching_evidence(self):
        cases = [("candidateSha", None), ("receiptSha", "bad"), ("receiptBlobSha", "bad"),
                 ("cycleDate", "2026-99-99"), ("snapshotObservedAt", "bad"),
                 ("schemaVersion", True), ("schemaVersion", 99)]
        for key, invalid in cases:
            with self.subTest(field=key, value=invalid):
                pair = self.pair()
                for value in pair:
                    value[key] = invalid
                self.assert_failed(pair)
        for probe in ("snapshotCli", "onlineCli"):
            for invalid in (True, None, "0", -1, 2):
                with self.subTest(probe=probe, exit=invalid):
                    pair = self.pair()
                    for value in pair:
                        value[probe]["exitCode"] = invalid
                    self.assert_failed(pair)

    def test_unverified_or_internally_inconsistent_online_ref_is_not_a_pass(self):
        for change in ("unverified", "false-stable", "false-changed"):
            with self.subTest(ref=change):
                pair = self.pair()
                for value in pair:
                    if change == "unverified":
                        value["onlineRefStatus"] = "UNVERIFIED"
                        value["onlineRefBefore"] = {"status": "FAIL", "error": "REF_OBSERVATION_UNAVAILABLE"}
                    elif change == "false-stable":
                        value["onlineRefAfter"]["sha"] = "d" * 40
                    else:
                        value["onlineRefStatus"] = "CHANGED"
                self.assert_failed(pair)

    def test_wrong_nested_types_produce_failure_reports(self):
        cases = [(key, invalid) for key in (
            "onlineRefBefore", "onlineRefAfter", "onlineCli", "snapshotCli", "nativeHttps", "curlFallback")
            for invalid in (None, [])]
        cases.extend([("onlineRefBefore", {"status": "PASS", "sha": []}),
                      ("onlineRefAfter", {"status": "PASS", "sha": []})])
        for key, invalid in cases:
            with self.subTest(field=key, value=invalid):
                pair = self.pair()
                for value in pair:
                    value[key] = copy.deepcopy(invalid)
                self.assert_failed(pair)
        for probe, field, invalid in (("onlineCli", "states", None), ("onlineCli", "states", []),
                                      ("onlineCli", "summary", []), ("snapshotCli", "results", None),
                                      ("snapshotCli", "results", [None]), ("snapshotCli", "summary", [])):
            with self.subTest(probe=probe, field=field, value=invalid):
                pair = self.pair()
                for value in pair:
                    value[probe][field] = copy.deepcopy(invalid)
                self.assert_failed(pair)


if __name__ == "__main__":
    unittest.main()
