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
import os
import re
import sys
import tempfile
from datetime import datetime, timedelta, timezone
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import urljoin, urlparse
from urllib.request import Request

if __package__:
    from .yoj_http import (YoJNetworkError, build_yoj_opener, classify_yoj_error,
                          read_public_get, source_state_for_reason, validate_yoj_url)
else:
    from yoj_http import (YoJNetworkError, build_yoj_opener, classify_yoj_error,
                         read_public_get, source_state_for_reason, validate_yoj_url)


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / "data" / "yoj-public-problems.json"
PUBLIC_INDEX_URL = "http://yoj.ruc.edu.cn/index.php/index/problem/index.html"
PUBLIC_INDEX_PAGE_URL = "http://yoj.ruc.edu.cn/index.php/index/problem/index/p/{page}.html"
EXPECTED_SCHEMA = 1
REQUEST_TIMEOUT = 30

ANCHOR_RE = re.compile(
    r"<a\b[^>]*?href\s*=\s*[\"']([^\"']*?/index\.php/(?:index/)?problem/detail/pno/(\d+)(?:\.html)?[^\"']*)[\"'][^>]*>(.*?)</a>",
    flags=re.IGNORECASE | re.DOTALL,
)
TAG_RE = re.compile(r"<[^>]+>")
PAGE_LINK_RE = re.compile(
    r"/index\.php/(?:index/)?problem/(?:index|list)(?:/p|/page)/?(\d+)(?:\.html)?$"
    r"|(?:^|[?&])(?:p|page)=(\d+)(?:&|$)",
    flags=re.IGNORECASE,
)


def utc_now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def checked_now() -> str:
    return datetime.now(timezone(timedelta(hours=8))).replace(microsecond=0).isoformat()


LOGIN_ROUTE_RE = re.compile(r"/(?:signin|login|login2)(?:\.html)?/?$", re.I)
LOGIN_TITLE_RE = re.compile(r"\blog\s*in\b|\bsign\s*in\b|登录|登陆", re.I)


class _PageSignals(HTMLParser):
    """Look at portal/form structure, never ordinary navigation link text."""

    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.in_title = False
        self.title_parts: list[str] = []
        self.in_form = False
        self.password_form = False
        self.campus_form = False

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if tag == "title":
            self.in_title = True
        elif tag == "form":
            self.in_form = True
            action = urlparse(urljoin(PUBLIC_INDEX_URL, attrs.get("action") or ""))
            if action.hostname in {"wvpn.ruc.edu.cn", "webvpn.ruc.edu.cn"}:
                self.campus_form = True
        elif tag == "input" and self.in_form and (attrs.get("type") or "").lower() == "password":
            self.password_form = True

    def handle_endtag(self, tag):
        if tag == "title":
            self.in_title = False
        elif tag == "form":
            self.in_form = False

    def handle_data(self, data):
        if self.in_title:
            self.title_parts.append(data)


def validate_public_page(page: str, final_url: str = PUBLIC_INDEX_URL) -> None:
    """Reject access portals on *every* page, including legacy pagination.

    An index with a login navigation link (or even an embedded login form)
    remains valid when its title and problem rows identify the index itself.
    """

    validate_yoj_url(final_url)
    if LOGIN_ROUTE_RE.search(urlparse(final_url).path):
        raise YoJNetworkError("YOJ_AUTH_REQUIRED")
    signals = _PageSignals()
    signals.feed(page)
    title = " ".join(signals.title_parts)
    if signals.campus_form or re.search(r"web\s*vpn", title, re.I):
        raise YoJNetworkError("YOJ_CAMPUS_ACCESS_REQUIRED")
    has_rows = bool(ANCHOR_RE.search(page))
    login_title = bool(LOGIN_TITLE_RE.search(title))
    if (signals.password_form and (not has_rows or login_title)) or (login_title and not has_rows):
        raise YoJNetworkError("YOJ_AUTH_REQUIRED")


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
            raise YoJNetworkError("YOJ_INVALID_PUBLIC_INDEX")
        problem_url = urljoin(PUBLIC_INDEX_URL, href)
        validate_yoj_url(problem_url)
        previous = rows.get(problem_no)
        current = {"problemNo": problem_no, "title": title, "problemUrl": problem_url}
        if previous is not None and previous != current:
            raise YoJNetworkError("YOJ_INVALID_PUBLIC_INDEX")
        rows[problem_no] = current
    if not rows:
        raise YoJNetworkError("YOJ_INVALID_PUBLIC_INDEX")
    return [rows[number] for number in sorted(rows)]


def public_page_links(page: str) -> dict[int, str]:
    """Return numbered public-index pages advertised by the server."""

    links: dict[int, str] = {}
    for href in re.findall(r"<a\b[^>]*href\s*=\s*[\"']([^\"']+)[\"'][^>]*>", page, flags=re.I | re.S):
        absolute = urljoin(PUBLIC_INDEX_URL, href)
        parsed = urlparse(absolute)
        # A login/user/help link with ?page=2 is not an index page.
        if not re.search(r"/index\.php/(?:index/)?problem/(?:index|list)(?:[/.]|$)", parsed.path, re.I):
            continue
        match = PAGE_LINK_RE.search(parsed.path)
        if not match:
            match = PAGE_LINK_RE.search(parsed.query)
        if not match:
            continue
        page_no = int(next(value for value in match.groups() if value is not None))
        if page_no > 0:
            links[page_no] = absolute
    return links


def fetch_page(url: str = PUBLIC_INDEX_URL) -> str:
    validate_yoj_url(url)
    request = Request(
        url,
        headers={"User-Agent": "RUC_YOJ-public-availability-audit/1.0"},
    )
    body, charset, final_url = read_public_get(build_yoj_opener(), request, REQUEST_TIMEOUT)
    page = body.decode(charset, errors="replace")
    validate_public_page(page, final_url)
    return page


def merge_rows(rows: dict[int, dict[str, object]], page_rows: list[dict[str, object]]) -> None:
    """Merge one page and reject conflicting representations of one problem."""

    for row in page_rows:
        problem_no = int(row["problemNo"])
        previous = rows.get(problem_no)
        if previous is not None and previous != row:
            raise YoJNetworkError("YOJ_INVALID_PUBLIC_INDEX")
        rows[problem_no] = row


def fetch_all_rows(max_pages: int = 100) -> tuple[list[dict[str, object]], int]:
    """Fetch every public-index page before calculating the daily ID set.

    YOJ installations have used both numbered server links and a legacy route
    without rendered pagination.  Follow the advertised graph first, then
    probe the legacy route until it is empty or repeats a page's ID set.  An
    empty aggregate is always an error so an outage cannot look like a clean
    no-new-problem run.
    """

    if max_pages < 1:
        raise ValueError("max_pages must be positive")
    first = fetch_page()
    validate_public_page(first)
    merged: dict[int, dict[str, object]] = {}
    merge_rows(merged, parse_rows(first))
    pages: dict[int, str] = {1: PUBLIC_INDEX_URL}
    pages.update({number: url for number, url in public_page_links(first).items() if number <= max_pages})
    fetched = 1
    visited = {1}
    queue = sorted(number for number in pages if number != 1)

    while queue:
        page_no = queue.pop(0)
        if page_no in visited or page_no > max_pages:
            continue
        page = fetch_page(pages[page_no])
        validate_public_page(page, pages[page_no])
        visited.add(page_no)
        fetched += 1
        merge_rows(merged, parse_rows(page))
        for linked_no, linked_url in public_page_links(page).items():
            if linked_no <= max_pages and linked_no not in pages:
                pages[linked_no] = linked_url
                queue.append(linked_no)
        queue.sort()

    if len(visited) == 1:
        previous_numbers = set(merged)
        for page_no in range(2, max_pages + 1):
            page = fetch_page(PUBLIC_INDEX_PAGE_URL.format(page=page_no))
            validate_public_page(page, PUBLIC_INDEX_PAGE_URL.format(page=page_no))
            fetched += 1
            page_rows = parse_rows(page) if ANCHOR_RE.search(page) else []
            current_numbers = {int(row["problemNo"]) for row in page_rows}
            if not current_numbers or current_numbers == previous_numbers:
                break
            merge_rows(merged, page_rows)
            previous_numbers = current_numbers

    if not merged:
        raise YoJNetworkError("YOJ_INVALID_PUBLIC_INDEX")
    return [merged[number] for number in sorted(merged)], fetched


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


def write_json_atomic(output: Path, payload: dict[str, object]) -> None:
    """Replace complete JSON in one step; a failed write leaves prior evidence."""

    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=output.parent,
                                         prefix=f".{output.name}.", suffix=".tmp", delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(json.dumps(payload, ensure_ascii=False, indent=2) + "\n")
            stream.flush()
            os.fsync(stream.fileno())
        temporary.replace(output)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def write_status(output: Path, reason: str, changed: bool | None) -> None:
    write_json_atomic(output, {
        "schemaVersion": 1,
        "checkedAt": checked_now(),
        "sourceState": source_state_for_reason(reason),
        "reason": reason,
        "snapshotChanged": changed,
    })


def write_snapshot(rows: list[dict[str, object]], output: Path, pages_fetched: int = 1) -> bool:
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
        "pagesFetched": pages_fetched,
        "problemListSha256": digest,
        "records": rows,
    }
    write_json_atomic(output, payload)
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="审计并保存 YOJ 当前公开题目列表快照")
    source = parser.add_mutually_exclusive_group()
    source.add_argument("--check", action="store_true", help="只检查已保存快照，不访问 YOJ")
    source.add_argument("--input-html", type=Path, help="使用已保存的公开索引 HTML，便于可重复生成")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT, help="快照输出路径")
    parser.add_argument("--status-output", type=Path, help="独立保存本次实时来源检查状态；不支持 --check 或 --input-html")
    parser.add_argument("--max-pages", type=int, default=100, help="公开题目列表最多扫描页数，默认 100")
    args = parser.parse_args()
    if args.status_output and (args.check or args.input_html or args.status_output.resolve() == args.output.resolve()):
        parser.error("--status-output requires a source audit and a path distinct from --output")

    reason = "NONE"
    changed = None
    result = 0
    try:
        if args.check:
            payload = json.loads(args.output.read_text(encoding="utf-8"))
            rows = validate_snapshot(payload)
            print(f"online availability snapshot ok: {len(rows)} public YOJ problems -> {args.output}")
            return 0
        if args.max_pages < 1:
            raise ValueError("max_pages must be positive")
        if args.input_html:
            page = args.input_html.read_text(encoding="utf-8", errors="replace")
            validate_public_page(page)
            rows = parse_rows(page)
            pages_fetched = 1
        else:
            rows, pages_fetched = fetch_all_rows(args.max_pages)
        changed = write_snapshot(rows, args.output, pages_fetched)
        action = "captured" if changed else "unchanged"
        print(
            f"{action} {len(rows)} public YOJ problems across {pages_fetched} pages -> {args.output}; "
            f"public-list sha256={digest_rows(rows)}"
        )
    except Exception as exc:
        reason = classify_yoj_error(exc)
        changed = None
        result = 1
        print(f"online availability audit failed: {reason}", file=sys.stderr)
    if args.status_output:
        try:
            write_status(args.status_output, reason, changed)
        except OSError:
            print("online availability status warning: YOJ_SOURCE_ERROR", file=sys.stderr)
    return result


if __name__ == "__main__":
    raise SystemExit(main())
