#!/usr/bin/env python3
"""Audit same-number YOJ statement drift without changing the release.

The daily synchronizer intentionally compares only problem numbers.  This
tool is the deliberately lower-frequency second line of defence: it reads
the current public problem set, normalizes each live statement with the same
``StatementConverter`` used by the repository build, and compares that
semantic body plus the problem-page limits with the local raw snapshot.

The result is written to ignored ``staging/problem-drift.json``.  A detected
change is a review queue entry, never an instruction to overwrite a frozen
``PUBLIC_READY`` statement or solution.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
import time
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path
from typing import Any
from urllib.parse import urlparse

from lxml import html

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))
from audit_online_availability import validate_snapshot  # noqa: E402
from build_initial import (  # noqa: E402
    StatementConverter,
    extract_statement_limits,
    json_load,
    normalize_inline_text,
    text_content,
)
from online_verify import YoJClient  # noqa: E402


DEFAULT_SNAPSHOT = ROOT / "data" / "yoj-public-problems.json"
DEFAULT_REPORT = ROOT / "staging" / "problem-drift.json"
EXPECTED_SCHEMA = 1


def utc_now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def sha256_bytes(content: bytes) -> str:
    return hashlib.sha256(content).hexdigest()


def sha256_text(content: str) -> str:
    return sha256_bytes(content.encode("utf-8"))


def decode_html(content: bytes) -> str:
    for encoding in ("utf-8", "gb18030", "big5"):
        try:
            return content.decode(encoding)
        except UnicodeDecodeError:
            continue
    return content.decode("utf-8", errors="replace")


def parse_document(content: bytes) -> Any:
    text = decode_html(content)
    if not text.strip():
        raise ValueError("EMPTY_HTML")
    return html.fromstring(text)


def page_path(url: str) -> str:
    parsed = urlparse(url)
    path = parsed.path or "/"
    return f"{path}?{parsed.query}" if parsed.query else path


def page_title(document: Any) -> str:
    for node in document.xpath("//h1|//h2|//title"):
        value = normalize_inline_text(text_content(node)).strip()
        if value and "YOJ" not in value.upper():
            return value
    return ""


def semantic_snapshot(content: bytes, problem_url: str) -> dict[str, Any]:
    """Return the stable fields used for drift decisions.

    ``statementBodySha256`` includes examples and input/output sections after
    the normalizer has removed page chrome.  Raw HTML is retained separately
    as evidence only; a template-only HTML change is therefore not enough to
    queue a re-verification.
    """

    document = parse_document(content)
    converter = StatementConverter(problem_url, download_assets=False)
    body = converter.convert(document)
    if not body:
        raise ValueError("EMPTY_STATEMENT_BODY")
    limits = extract_statement_limits(document)
    return {
        "title": page_title(document),
        "limits": limits,
        "statementBodySha256": sha256_text(body),
        "statementBodyBytes": len(body.encode("utf-8")),
        "converterWarnings": sorted(set(converter.warnings)),
    }


def relative_or_absolute(path: Path) -> str:
    try:
        return path.relative_to(ROOT).as_posix()
    except ValueError:
        return str(path)


def raw_problem_path(record: dict[str, Any]) -> Path | None:
    folder = str(record.get("folder") or "").strip()
    if not folder:
        return None
    raw_dir = ROOT / "代码库" / folder
    padded = str(record.get("problemNo") or "").zfill(4)
    metadata_ref = str((record.get("archive") or {}).get("metadata") or "").strip()
    if metadata_ref:
        metadata_path = ROOT / metadata_ref
        try:
            metadata = json_load(metadata_path)
            problem_html = str((metadata.get("files") or {}).get("problemHtml") or "").strip()
            if problem_html:
                candidate = raw_dir / problem_html
                if candidate.is_file():
                    return candidate
        except (OSError, TypeError, ValueError, json.JSONDecodeError):
            pass
    candidates = sorted(raw_dir.glob(f"{padded}_题目原文.html"))
    return candidates[0] if candidates else None


def frozen_statement_hash(record: dict[str, Any]) -> str | None:
    statement = str((record.get("public") or {}).get("statement") or "").strip()
    path = ROOT / statement if statement else None
    if path is None or not path.is_file():
        return None
    return sha256_bytes(path.read_bytes())


def load_local_records() -> dict[int, dict[str, Any]]:
    payload = json_load(ROOT / "data" / "problems.json")
    return {
        int(row["problemNo"]): row
        for row in (payload.get("records") or [])
        if str(row.get("problemNo", "")).isdigit()
    }


def load_public_rows(path: Path) -> list[dict[str, Any]]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    return [dict(row) for row in validate_snapshot(payload)]


def compare_fields(local: dict[str, Any], live: dict[str, Any]) -> list[str]:
    changed: list[str] = []
    if local.get("statementBodySha256") != live.get("statementBodySha256"):
        changed.append("statement_body_or_samples")
    if local.get("limits") != live.get("limits"):
        changed.append("limits")
    local_title = str(local.get("title") or "").strip()
    live_title = str(live.get("title") or "").strip()
    if local_title and live_title and local_title != live_title:
        changed.append("title")
    return changed


def make_baseline(record: dict[str, Any], raw_path: Path) -> dict[str, Any]:
    content = raw_path.read_bytes()
    local = semantic_snapshot(content, str(record.get("problemUrl") or ""))
    local["rawHtmlSha256"] = sha256_bytes(content)
    local["rawPath"] = relative_or_absolute(raw_path)
    local["frozenStatementSha256"] = frozen_statement_hash(record)
    return local


def make_report(rows: list[dict[str, Any]], started_at: str, finished_at: str, snapshot: Path) -> dict[str, Any]:
    counts = Counter(str(row.get("status") or "UNKNOWN") for row in rows)
    return {
        "schemaVersion": EXPECTED_SCHEMA,
        "purpose": "low-frequency same-number YOJ statement/limit drift audit; release remains frozen",
        "updatedAt": finished_at,
        "startedAt": started_at,
        "scope": "current_public_problem_numbers",
        "publicSnapshot": relative_or_absolute(snapshot),
        "auditedProblemCount": len(rows),
        "summary": dict(sorted(counts.items())),
        "records": sorted(rows, key=lambda row: int(row.get("problemNo", 0))),
    }


def write_report(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def validate_report(payload: dict[str, Any]) -> int:
    if payload.get("schemaVersion") != EXPECTED_SCHEMA:
        raise ValueError(f"unsupported report schema: {payload.get('schemaVersion')!r}")
    records = payload.get("records")
    if not isinstance(records, list):
        raise ValueError("report records must be a list")
    seen: set[int] = set()
    for row in records:
        if not isinstance(row, dict) or not str(row.get("problemNo", "")).isdigit():
            raise ValueError(f"invalid report record: {row!r}")
        problem_no = int(row["problemNo"])
        if problem_no in seen:
            raise ValueError(f"duplicate report problem: {problem_no}")
        seen.add(problem_no)
        if row.get("status") not in {
            "UNCHANGED",
            "DRIFT_DETECTED",
            "BASELINE_MISSING",
            "BASELINE_INVALID",
            "FETCH_FAILED",
            "NOT_CURRENTLY_PUBLIC",
        }:
            raise ValueError(f"invalid report status for {problem_no}: {row.get('status')!r}")
    return len(records)


def audit(args: argparse.Namespace) -> int:
    snapshot_path = args.public_snapshot.resolve()
    report_path = args.report.resolve()
    public_rows = load_public_rows(snapshot_path)
    public_by_no = {int(row["problemNo"]): row for row in public_rows}
    if args.problem_nos:
        selected = sorted(set(args.problem_nos))
    else:
        selected = sorted(public_by_no)
        if args.max_problems:
            selected = selected[: args.max_problems]

    local_records = load_local_records()
    client = YoJClient()
    limiter_last = 0.0
    started_at = utc_now()
    results: list[dict[str, Any]] = []
    for problem_no in selected:
        public_row = public_by_no.get(problem_no)
        if public_row is None:
            results.append(
                {
                    "problemNo": problem_no,
                    "status": "NOT_CURRENTLY_PUBLIC",
                    "action": "NO_DAILY_REFRESH",
                    "checkedAt": utc_now(),
                }
            )
            continue
        record = local_records.get(problem_no)
        checked_at = utc_now()
        base = {
            "problemNo": problem_no,
            "title": str(public_row.get("title") or ""),
            "problemUrl": str(public_row.get("problemUrl") or ""),
            "checkedAt": checked_at,
            "releaseFrozen": str((record or {}).get("public", {}).get("status") or "") == "PUBLIC_READY",
        }
        if record is None:
            results.append({**base, "status": "BASELINE_MISSING", "action": "QUEUE_CAPTURE"})
            continue
        raw_path = raw_problem_path(record)
        if raw_path is None:
            results.append({**base, "status": "BASELINE_MISSING", "action": "QUEUE_CAPTURE"})
            continue
        try:
            local = make_baseline(record, raw_path)
        except (OSError, TypeError, ValueError, json.JSONDecodeError) as exc:
            results.append(
                {
                    **base,
                    "status": "BASELINE_INVALID",
                    "action": "QUEUE_REVIEW",
                    "error": exc.__class__.__name__,
                    "rawPath": relative_or_absolute(raw_path),
                }
            )
            continue
        try:
            wait_for = float(args.request_interval) - (time.monotonic() - limiter_last)
            if wait_for > 0:
                time.sleep(wait_for)
            live_content = client.request(page_path(str(public_row["problemUrl"])))
            limiter_last = time.monotonic()
            live = semantic_snapshot(live_content.encode("utf-8"), str(public_row["problemUrl"]))
            live["rawHtmlSha256"] = sha256_text(live_content)
            changed_fields = compare_fields(local, live)
            status = "DRIFT_DETECTED" if changed_fields else "UNCHANGED"
            results.append(
                {
                    **base,
                    "status": status,
                    "action": "QUEUE_REVERIFY" if changed_fields else "NO_ACTION",
                    "changedFields": changed_fields,
                    "baseline": local,
                    "live": live,
                    "rawHtmlChanged": local.get("rawHtmlSha256") != live.get("rawHtmlSha256"),
                    "note": (
                        "只记录漂移并冻结现有发布内容；后续须重新抓取题面、代码/样例并经过完整门禁。"
                        if changed_fields
                        else "规范化题面正文和题面限制未变化；原始 HTML 的模板差异不单独触发更新。"
                    ),
                }
            )
        except Exception as exc:  # noqa: BLE001 - retain per-problem evidence and continue
            results.append(
                {
                    **base,
                    "status": "FETCH_FAILED",
                    "action": "RETRY_LATER",
                    "error": f"{exc.__class__.__name__}: {exc}",
                    "baseline": local,
                }
            )
    finished_at = utc_now()
    report = make_report(results, started_at, finished_at, snapshot_path)
    write_report(report_path, report)
    print(json.dumps({key: report[key] for key in ("updatedAt", "auditedProblemCount", "summary")}, ensure_ascii=False))
    return 3 if any(row.get("status") in {"FETCH_FAILED", "BASELINE_INVALID"} for row in results) else 0


def main() -> int:
    parser = argparse.ArgumentParser(description="低频审计 YOJ 同题号题面、限制和样例语义漂移")
    parser.add_argument("--check", action="store_true", help="只校验已有漂移报告，不访问 YOJ")
    parser.add_argument("--public-snapshot", type=Path, default=DEFAULT_SNAPSHOT, help="当前公开题目列表快照")
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT, help="忽略目录中的漂移报告")
    parser.add_argument("--problem", dest="problem_nos", action="append", type=int, help="只审计指定题号")
    parser.add_argument("--max-problems", type=int, default=0, help="默认 0 表示审计快照中的全部公开题目")
    parser.add_argument("--request-interval", type=float, default=1.0, help="题面请求最小间隔秒数")
    args = parser.parse_args()
    if args.max_problems < 0 or args.request_interval < 0:
        raise SystemExit("题目数和请求间隔不能为负数")
    try:
        if args.check:
            payload = json.loads(args.report.read_text(encoding="utf-8"))
            count = validate_report(payload)
            print(f"problem drift report ok: {count} records -> {args.report}")
            return 0
        return audit(args)
    except (OSError, TypeError, ValueError, json.JSONDecodeError) as exc:
        print(f"problem drift audit failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
