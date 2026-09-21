#!/usr/bin/env python3
"""Track YOJ public-list visibility transitions without touching problem data.

The public index is an independent evidence source.  This tool compares the
latest validated public snapshot with the last successfully completed daily
cycle and the local historical problem set.  It reports two reversible
transitions:

* a locally archived problem leaving the YOJ public index;
* a locally archived problem appearing in the YOJ public index again.

The state file is ignored runtime state.  It is committed only after the
corresponding lightweight projection or new-problem pipeline has completed;
that way a failed README/catalog refresh is retried on the next run.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import sys
import tempfile
from pathlib import Path
from typing import Any, Iterable

try:
    from audit_online_availability import validate_snapshot
except ImportError:  # pragma: no cover - supports package-style imports
    from tools.audit_online_availability import validate_snapshot


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SNAPSHOT = ROOT / "data" / "yoj-public-problems.json"
DEFAULT_STATE = ROOT / ".yoj-sync" / "public-visibility-state.json"
DEFAULT_REPORT = ROOT / ".yoj-sync" / "public-visibility-report.json"
MANIFEST_PATH = ROOT / "代码库" / "AC抓取清单.json"
PROBLEMS_PATH = ROOT / "data" / "problems.json"
LOCAL_PROBLEM_ROOTS = (ROOT / "代码库", ROOT / "题解")
STATE_SCHEMA_VERSION = 1


def safe_int(value: Any) -> int | None:
    try:
        number = int(str(value))
    except (TypeError, ValueError):
        return None
    return number if number > 0 else None


def numbers_from_records(records: Iterable[dict[str, Any]]) -> set[int]:
    return {
        number
        for row in records
        if isinstance(row, dict)
        for number in [safe_int(row.get("problemNo"))]
        if number is not None
    }


def load_json(path: Path) -> dict[str, Any]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(payload, dict):
        raise ValueError(f"JSON root must be an object: {path}")
    return payload


def load_public_snapshot(path: Path) -> tuple[set[int], dict[str, Any]]:
    payload = load_json(path)
    rows = validate_snapshot(payload)
    return {int(row["problemNo"]) for row in rows}, payload


def load_local_problem_numbers(
    problems_path: Path = PROBLEMS_PATH,
    manifest_path: Path = MANIFEST_PATH,
    problem_roots: tuple[Path, ...] = LOCAL_PROBLEM_ROOTS,
) -> set[int]:
    """Return the union used by the daily new-problem gate."""

    numbers: set[int] = set()
    for path, key in ((problems_path, "records"), (manifest_path, "problems")):
        if not path.is_file():
            continue
        try:
            payload = load_json(path)
        except (OSError, TypeError, ValueError, json.JSONDecodeError):
            continue
        numbers.update(numbers_from_records(payload.get(key) or []))
    for problem_root in problem_roots:
        if not problem_root.is_dir():
            continue
        for child in problem_root.iterdir():
            if not child.is_dir():
                continue
            prefix = child.name.split("_", 1)[0]
            number = safe_int(prefix)
            if number is not None:
                numbers.add(number)
    return numbers


def load_records(path: Path = PROBLEMS_PATH) -> list[dict[str, Any]]:
    if not path.is_file():
        return []
    try:
        payload = load_json(path)
    except (OSError, TypeError, ValueError, json.JSONDecodeError):
        return []
    return [row for row in (payload.get("records") or []) if isinstance(row, dict)]


def accepted_problem_numbers(records: Iterable[dict[str, Any]]) -> set[int]:
    """Find local records with an Accepted signal, without changing them."""

    accepted: set[int] = set()
    for record in records:
        number = safe_int(record.get("problemNo"))
        if number is None:
            continue
        public = record.get("public") or {}
        archive = record.get("archive") or {}
        accepted_run = archive.get("acceptedRun") or {}
        if (
            public.get("status") == "PUBLIC_READY"
            or public.get("onlineVerification") == "ONLINE_ACCEPTED"
            or accepted_run.get("status") == "Accepted"
        ):
            accepted.add(number)
    return accepted


def public_ready_problem_numbers(records: Iterable[dict[str, Any]]) -> set[int]:
    return {
        number
        for record in records
        for number in [safe_int(record.get("problemNo"))]
        if number is not None and (record.get("public") or {}).get("status") == "PUBLIC_READY"
    }


def digest_numbers(numbers: Iterable[int]) -> str:
    normalized = "\n".join(str(number) for number in sorted(set(numbers)))
    return hashlib.sha256(normalized.encode("utf-8")).hexdigest()


def load_state(path: Path) -> dict[str, Any] | None:
    if not path.is_file():
        return None
    payload = load_json(path)
    if payload.get("schemaVersion") != STATE_SCHEMA_VERSION:
        raise ValueError(f"unsupported visibility state schema: {payload.get('schemaVersion')!r}")
    public_numbers = payload.get("publicProblemNumbers")
    known_numbers = payload.get("knownProblemNumbers")
    if not isinstance(public_numbers, list) or not isinstance(known_numbers, list):
        raise ValueError("visibility state must contain publicProblemNumbers and knownProblemNumbers lists")
    parsed_public = sorted({number for value in public_numbers if (number := safe_int(value)) is not None})
    parsed_known = sorted({number for value in known_numbers if (number := safe_int(value)) is not None})
    if payload.get("publicProblemListSha256") != digest_numbers(parsed_public):
        raise ValueError("visibility state publicProblemListSha256 does not match its IDs")
    return {
        **payload,
        "publicProblemNumbers": parsed_public,
        "knownProblemNumbers": parsed_known,
    }


def compare_visibility(
    current_public: set[int],
    local_numbers: set[int],
    accepted_numbers: set[int],
    public_ready_numbers: set[int],
    previous_state: dict[str, Any] | None,
    snapshot: dict[str, Any],
) -> dict[str, Any]:
    """Build a deterministic, public-safe transition report."""

    baseline = previous_state is None
    previous_public = set(previous_state.get("publicProblemNumbers", [])) if previous_state else set()
    previous_known = set(previous_state.get("knownProblemNumbers", [])) if previous_state else set()
    locally_archived = sorted(local_numbers - current_public)
    if baseline:
        archived = []
        reopened = []
    else:
        archived = sorted((previous_public - current_public) & previous_known)
        reopened = sorted((current_public - previous_public) & previous_known)
    accepted_archived = sorted(set(archived) & accepted_numbers)
    ready_archived = sorted(set(archived) & public_ready_numbers)
    return {
        "schemaVersion": STATE_SCHEMA_VERSION,
        "baseline": baseline,
        "hasVisibilityTransitions": bool(archived or reopened),
        "action": "ESTABLISH_BASELINE" if baseline else "REFRESH_VISIBILITY_PROJECTION" if archived or reopened else "SKIP_ALL",
        "snapshotCapturedAt": snapshot.get("capturedAt"),
        "snapshotSha256": snapshot.get("problemListSha256"),
        "publicProblemCount": len(current_public),
        "localProblemCount": len(local_numbers),
        "currentlyPublicLocalCount": len(current_public & local_numbers),
        "locallyArchivedProblemCount": len(locally_archived),
        "locallyArchivedProblemNumbers": locally_archived,
        "archivedProblemNumbers": archived,
        "onlineAcceptedArchivedProblemNumbers": accepted_archived,
        "publicReadyArchivedProblemNumbers": ready_archived,
        "reopenedProblemNumbers": reopened,
        "newPublicProblemNumbers": sorted(current_public - local_numbers),
        "previousPublicProblemCount": len(previous_public),
        "previousKnownProblemCount": len(previous_known),
    }


def state_payload(public_numbers: set[int], local_numbers: set[int], snapshot: dict[str, Any]) -> dict[str, Any]:
    ordered_public = sorted(public_numbers)
    return {
        "schemaVersion": STATE_SCHEMA_VERSION,
        "purpose": "last successfully projected YOJ public visibility and local problem baseline",
        "capturedAt": snapshot.get("capturedAt"),
        "publicProblemListSha256": digest_numbers(ordered_public),
        "publicProblemNumbers": ordered_public,
        "knownProblemNumbers": sorted(local_numbers),
    }


def atomic_write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary_name = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    temporary = Path(temporary_name)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as handle:
            json.dump(payload, handle, ensure_ascii=False, indent=2)
            handle.write("\n")
        temporary.replace(path)
    finally:
        if temporary.exists():
            temporary.unlink()


def main() -> int:
    parser = argparse.ArgumentParser(description="比较 YOJ 当前公开列表与本地归档的可见性变化")
    parser.add_argument("--snapshot", type=Path, default=DEFAULT_SNAPSHOT)
    parser.add_argument("--state", type=Path, default=DEFAULT_STATE)
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    parser.add_argument(
        "--commit",
        action="store_true",
        help="在当前调度阶段成功后保存新的可见性基线；不改变公开题目数据",
    )
    args = parser.parse_args()
    try:
        current_public, snapshot = load_public_snapshot(args.snapshot)
        local_numbers = load_local_problem_numbers()
        records = load_records()
        accepted_numbers = accepted_problem_numbers(records)
        public_ready_numbers = public_ready_problem_numbers(records)
        previous_state = load_state(args.state)
        report = compare_visibility(
            current_public,
            local_numbers,
            accepted_numbers,
            public_ready_numbers,
            previous_state,
            snapshot,
        )
        report["stateCommitted"] = False
        atomic_write_json(args.report, report)
        if args.commit:
            atomic_write_json(args.state, state_payload(current_public, local_numbers, snapshot))
            report["stateCommitted"] = True
            atomic_write_json(args.report, report)
        print(json.dumps(report, ensure_ascii=False))
        return 0
    except (OSError, TypeError, ValueError, KeyError, json.JSONDecodeError) as exc:
        print(f"public visibility audit failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
