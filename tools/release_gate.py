#!/usr/bin/env python3
"""Shared, offline-only gates for YOJ candidate submission and publication.

The module never logs in and never sends a request.  It only compares the
candidate selected for submission with the local validation reports and the
ignored online evidence.  Keeping these checks in one place prevents the
submission runner and the public-release step from silently using different
files.
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Any, Iterable


ROOT = Path(__file__).resolve().parents[1]
RAW_ROOT = ROOT / "代码库"
STAGING_ROOT = ROOT / "staging"
REPORT_PATHS = {
    "validation": STAGING_ROOT / "local-validation-report.json",
    "compile": STAGING_ROOT / "local-compile-report.json",
    "samples": STAGING_ROOT / "sample-test-report.json",
    "sanitization": STAGING_ROOT / "sanitization-report.json",
}

_CANDIDATE_ROOTS = (
    STAGING_ROOT / "cleaned-code",
    STAGING_ROOT / "repaired-code",
    STAGING_ROOT / "cpp17-portable",
    STAGING_ROOT / "cpp17-compatible",
)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def relative(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def candidate_path(raw_path: Path) -> Path:
    """Return the highest-priority existing candidate for a raw archive file."""

    raw_path = raw_path.resolve()
    relative_path = raw_path.relative_to(RAW_ROOT.resolve())
    for root in _CANDIDATE_ROOTS:
        candidate = root / relative_path
        if candidate.is_file():
            return candidate
    return raw_path


def candidate_paths(record: dict[str, Any]) -> tuple[Path, Path]:
    archive = record.get("archive") or {}
    complete_raw = ROOT / str(archive.get("completeCode") or "")
    direct_raw = ROOT / str(archive.get("directlySubmittableCode") or "")
    return candidate_path(complete_raw), candidate_path(direct_raw)


def _load(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _rows(path: Path, key: str) -> dict[str, dict[str, Any]]:
    if not path.is_file():
        return {}
    try:
        payload = _load(path)
    except (OSError, TypeError, ValueError, json.JSONDecodeError):
        return {}
    return {
        str(item.get("problemNo")): item
        for item in (payload.get(key) or [])
        if item.get("problemNo") is not None
    }


def report_rows(name: str) -> dict[str, dict[str, Any]]:
    if name == "sanitization":
        if not REPORT_PATHS[name].is_file():
            return {}
        try:
            payload = _load(REPORT_PATHS[name])
        except (OSError, TypeError, ValueError, json.JSONDecodeError):
            return {}
        return {
            str(item.get("candidate") or item.get("source") or index): item
            for index, item in enumerate(payload.get("fileRecords") or [])
        }
    key = "results"
    return _rows(REPORT_PATHS[name], key)


def _find_file_row(rows: Iterable[dict[str, Any]], paths: set[str]) -> dict[str, Any] | None:
    for row in rows:
        if str(row.get("candidate") or row.get("source") or "") in paths:
            return row
    return None


def local_gate_reasons(problem_no: int, complete: Path, direct: Path) -> list[str]:
    """Return blocking reasons for one candidate.

    ``NO_SAMPLE`` is explicitly allowed: it means that the statement parser
    could not safely extract a sample, not that the candidate passed a test.
    The online judge remains the required correctness proof in that case.
    """

    reasons: list[str] = []
    if not complete.is_file() or not direct.is_file():
        return ["CANDIDATE_FILE_MISSING"]

    complete_rel = relative(complete)
    direct_rel = relative(direct)
    complete_sha = sha256_file(complete)
    direct_sha = sha256_file(direct)

    for name in ("validation", "compile"):
        row = report_rows(name).get(str(problem_no))
        if not row:
            reasons.append(f"{name.upper()}_REPORT_MISSING")
            continue
        recorded_sha = str(row.get("candidateSha256") or "")
        if not recorded_sha or recorded_sha != complete_sha:
            reasons.append(f"{name.upper()}_HASH_STALE")
        if row.get("category") != "PASS":
            reasons.append(f"{name.upper()}_{row.get('category') or 'UNKNOWN'}")

    sample = report_rows("samples").get(str(problem_no))
    if not sample:
        reasons.append("SAMPLE_REPORT_MISSING")
    else:
        category = str(sample.get("category") or "UNKNOWN")
        if category not in {"PASS", "NO_SAMPLE"}:
            reasons.append(f"SAMPLE_{category}")
        if category == "PASS":
            recorded_sha = str(sample.get("candidateSha256") or "")
            if not recorded_sha or recorded_sha != complete_sha:
                reasons.append("SAMPLE_HASH_STALE")

    sanitization = report_rows("sanitization")
    complete_clean = _find_file_row(sanitization.values(), {complete_rel})
    direct_clean = _find_file_row(sanitization.values(), {direct_rel})
    for label, row, expected_sha in (
        ("COMPLETE", complete_clean, complete_sha),
        ("DIRECT", direct_clean, direct_sha),
    ):
        if not row:
            reasons.append(f"SANITIZATION_{label}_REPORT_MISSING")
            continue
        if str(row.get("candidateSha256") or "") != expected_sha:
            reasons.append(f"SANITIZATION_{label}_HASH_STALE")
        if row.get("remainingConfiguredMarkers"):
            reasons.append(f"SANITIZATION_{label}_MARKERS_REMAIN")

    return sorted(set(reasons))


def online_ready_reasons(row: dict[str, Any], direct: Path) -> list[str]:
    """Return blocking reasons for the online evidence of one candidate."""

    reasons: list[str] = []
    if str(row.get("status") or "") != "Accepted":
        reasons.append(f"ONLINE_{row.get('status') or 'UNKNOWN'}")
    if not direct.is_file():
        reasons.append("DIRECT_CANDIDATE_MISSING")
    else:
        candidate_sha = str(row.get("candidateSha256") or "")
        if not candidate_sha or candidate_sha != sha256_file(direct):
            reasons.append("ONLINE_CANDIDATE_HASH_MISMATCH")

    round_trip = row.get("roundTrip") or {}
    if round_trip.get("status") != "VISIBLE":
        reasons.append("SOURCE_RECAPTURE_NOT_VISIBLE")
    if not round_trip.get("sourceVisibleInSubmissionDetail"):
        reasons.append("SOURCE_DETAIL_NOT_VISIBLE")
    exact = bool(round_trip.get("exactByteMatch"))
    canonical = bool(round_trip.get("canonicalByteMatch"))
    if not (exact or canonical):
        reasons.append("SOURCE_MISMATCH")
    return sorted(set(reasons))


def is_public_ready(row: dict[str, Any], complete: Path, direct: Path, problem_no: int) -> tuple[bool, list[str]]:
    reasons = local_gate_reasons(problem_no, complete, direct)
    reasons.extend(online_ready_reasons(row, direct))
    return not reasons, sorted(set(reasons))
