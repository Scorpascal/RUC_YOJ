#!/usr/bin/env python3
"""Capture and validate the public problem index exposed by YOJ.

The repository has several deliberately independent states: a topic can be
public on YOJ, have a locally archived statement, and still not have a
publishable Accepted solution in this repository.  This tool records only the
public-index evidence needed to keep those states separate.  It never logs in
and never submits code.

The generated JSON is a small, public-safe snapshot.  It does not contain the
HTML response, cookies, account information, or submission records.
"""

from __future__ import annotations

import argparse
import hashlib
import html
import json
import re
import sys
from datetime import datetime, timezone
from pathlib import Path
from urllib.parse import urljoin
from urllib.request import Request, urlopen


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / "data" / "yoj-public-problems.json"
PUBLIC_INDEX_URL = "http://yoj.ruc.edu.cn/index.php/index/problem/index.html"
EXPECTED_SCHEMA = 1
REQUEST_TIMEOUT = 30

ANCHOR_RE = re.compile(
    r"<a\b[^>]*?href\s*=\s*[\"']([^\"']*?/index\.php/(?:index/)?problem/detail/pno/(\d+)(?:\.html)?[^\"']*)[\"'][^>]*>(.*?)</a>",
    flags=re.IGNORECASE | re.DOTALL,
)
TAG_RE = re.compile(r"<[^>]+>")


def utc_now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def clean_title(value: str) -> str:
    value = TAG_RE.sub(" ", value)
    value = html.unescape(value).replace("\xa0", " ")
    return re.sub(r"\s+", " ", value).strip()


def parse_rows(page: str) -> list[dict[str, object]]:
    """Parse the public table without depending on its pagination JavaScript."""

    rows: dict[int, dict[str, object]] = {}
    for match in ANCHOR_RE.finditer(page):
        href, raw_number, raw_title = match.groups()
        problem_no = int(raw_number)
        title = clean_title(raw_title)
        if not title:
            raise ValueError(f"public index has an empty title for problem {problem_no}")
        problem_url = urljoin(PUBLIC_INDEX_URL, href)
        previous = rows.get(problem_no)
        current = {"problemNo": problem_no, "title": title, "problemUrl": problem_url}
        if previous is not None and previous != current:
            raise ValueError(f"public index has conflicting rows for problem {problem_no}")
        rows[problem_no] = current
    if not rows:
        raise ValueError("YOJ public index yielded no problem rows; refusing to record an empty snapshot")
    return [rows[number] for number in sorted(rows)]


def fetch_page() -> str:
    request = Request(
        PUBLIC_INDEX_URL,
        headers={"User-Agent": "RUC_YOJ-public-availability-audit/1.0"},
    )
    with urlopen(request, timeout=REQUEST_TIMEOUT) as response:
        body = response.read()
        charset = response.headers.get_content_charset() or "utf-8"
    return body.decode(charset, errors="replace")


def digest_rows(rows: list[dict[str, object]]) -> str:
    normalized = "\n".join(
        f"{row['problemNo']}\t{row['title']}\t{row['problemUrl']}" for row in rows
    )
    return hashlib.sha256(normalized.encode("utf-8")).hexdigest()


def validate_snapshot(payload: dict[str, object]) -> list[dict[str, object]]:
    if payload.get("schemaVersion") != EXPECTED_SCHEMA:
        raise ValueError(f"unsupported snapshot schema: {payload.get('schemaVersion')!r}")
    if payload.get("source") != PUBLIC_INDEX_URL:
        raise ValueError("snapshot source is not the canonical YOJ public index")
    rows = payload.get("records")
    if not isinstance(rows, list):
        raise ValueError("snapshot records must be a list")
    parsed: list[dict[str, object]] = []
    seen: set[int] = set()
    for row in rows:
        if not isinstance(row, dict):
            raise ValueError("snapshot contains a non-object record")
        try:
            problem_no = int(row["problemNo"])
            title = str(row["title"]).strip()
            problem_url = str(row["problemUrl"]).strip()
        except (KeyError, TypeError, ValueError) as exc:
            raise ValueError(f"invalid snapshot record: {row!r}") from exc
        if problem_no <= 0 or not title or not problem_url or problem_no in seen:
            raise ValueError(f"invalid or duplicate snapshot problem: {row!r}")
        seen.add(problem_no)
        parsed.append({"problemNo": problem_no, "title": title, "problemUrl": problem_url})
    if not parsed:
        raise ValueError("snapshot contains no public problems")
    if payload.get("publicCount") != len(parsed):
        raise ValueError("snapshot publicCount does not match records")
    if payload.get("problemListSha256") != digest_rows(parsed):
        raise ValueError("snapshot problemListSha256 does not match records")
    return parsed


def write_snapshot(rows: list[dict[str, object]], output: Path) -> bool:
    """Persist a snapshot only when the public-list evidence changed.

    The daily scheduler may check the public index every run.  Rewriting the
    timestamp for an unchanged list would create a permanent dirty worktree
    and would make a no-new-problem run look like a content update.
    """

    digest = digest_rows(rows)
    if output.is_file():
        try:
            previous = json.loads(output.read_text(encoding="utf-8"))
            previous_rows = validate_snapshot(previous)
        except (OSError, TypeError, ValueError, json.JSONDecodeError):
            previous_rows = None
        if previous_rows == rows and previous.get("problemListSha256") == digest:
            return False
    payload = {
        "schemaVersion": EXPECTED_SCHEMA,
        "capturedAt": utc_now(),
        "source": PUBLIC_INDEX_URL,
        "publicCount": len(rows),
        "problemListSha256": digest,
        "records": rows,
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="审计并保存 YOJ 当前公开题目列表快照")
    source = parser.add_mutually_exclusive_group()
    source.add_argument("--check", action="store_true", help="只检查已保存快照，不访问 YOJ")
    source.add_argument("--input-html", type=Path, help="使用已保存的公开索引 HTML，便于可重复生成")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT, help="快照输出路径")
    args = parser.parse_args()

    try:
        if args.check:
            payload = json.loads(args.output.read_text(encoding="utf-8"))
            rows = validate_snapshot(payload)
            print(f"online availability snapshot ok: {len(rows)} public YOJ problems -> {args.output}")
            return 0
        page = args.input_html.read_text(encoding="utf-8", errors="replace") if args.input_html else fetch_page()
        rows = parse_rows(page)
        changed = write_snapshot(rows, args.output)
        action = "captured" if changed else "unchanged"
        print(f"{action} {len(rows)} public YOJ problems -> {args.output}; id-set sha256={digest_rows(rows)}")
        return 0
    except (OSError, TypeError, ValueError, json.JSONDecodeError) as exc:
        print(f"online availability audit failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
