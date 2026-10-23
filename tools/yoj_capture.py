#!/usr/bin/env python3
"""Capture new public topics and Accepted source for selected pending topics.

This is a read-only crawler with respect to the judge: it performs GETs for
submission/problem/detail pages and the authenticated getcode read endpoint;
it never calls the submit endpoint.  Cookies live only in the process, and
the login password must be injected by the scheduler from an external secret
store.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any
from urllib.parse import urljoin, urlparse

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))
from audit_online_availability import validate_snapshot  # noqa: E402
from online_verify import YoJClient, clean_text, parse_submission_rows  # noqa: E402
from yoj_context import Links, collect_problem_paths, discover_account_contexts, is_access_denied, problem_path as contextual_problem_path  # noqa: E402
from yoj_http import classify_yoj_error  # noqa: E402


MANIFEST_PATH = ROOT / "代码库" / "AC抓取清单.json"
DATA_PROBLEMS_PATH = ROOT / "data" / "problems.json"
PUBLIC_READY_PATH = ROOT / "data" / "public-ready.json"
RAW_ROOT = ROOT / "代码库"
LOCAL_PROBLEM_ROOTS = (RAW_ROOT, ROOT / "题解")
STATE_DIR = ROOT / ".yoj-sync"
CAPTURE_RESULT_PATH = STATE_DIR / "capture-result.json"
BASE = "http://yoj.ruc.edu.cn"
PUBLIC_PROBLEM_INDEX = "/index.php/index/problem/index.html"
PUBLIC_PROBLEM_PAGE = "/index.php/index/problem/index/p/{page}.html"
SUBMISSION_INDEX = "/index.php/index/submissions/index.html"
SUBMISSION_PAGE = "/index.php/submissions/index/p/{page}.html"
PROBLEM_DIR_RE = re.compile(r"^(\d+)(?:_|$)")
PUBLIC_PROBLEM_DETAIL_RE = re.compile(
    r"/index\.php/(?:index/)?problem/detail/pno/(\d+)(?:\.html)?(?:[\"'#?]|$)",
    re.I,
)


def utc_now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def digest_bytes(content: bytes) -> str:
    return hashlib.sha256(content).hexdigest()


def atomic_write(path: Path, content: bytes) -> bool:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.is_file() and path.read_bytes() == content:
        return False
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_bytes(content)
    temporary.replace(path)
    return True


def safe_title(title: str) -> str:
    value = re.sub(r"\s+", "_", title.strip())
    value = re.sub(r'[\\/:*?"<>|]+', "_", value)
    return value.strip(" ._") or "未命名题目"


def language_extension(language: str) -> str:
    value = language.lower().strip()
    if value in {"c", "c-noilinux"} or value.startswith("c-"):
        return ".c"
    if value.startswith("cpp") or value in {"c++", "cc", "cxx"}:
        return ".cpp"
    if value.startswith("python") or value in {"py", "py3"}:
        return ".py"
    return ".txt"


def page_numbers(page: str) -> list[int]:
    values = {int(item) for item in re.findall(r"/index\.php/(?:index/)?submissions/index/p/(\d+)\.html", page)}
    return sorted(values)


def public_problem_numbers(page: str) -> set[int]:
    """Extract problem numbers from the public problem index page."""

    numbers = {int(value) for value in PUBLIC_PROBLEM_DETAIL_RE.findall(page)}
    # Keep a narrow fallback for installations which omit ``index/`` from
    # links while retaining the same detail/pno route.
    if not numbers:
        numbers = {
            int(value)
            for value in re.findall(
                r"/index\.php/(?:index/)?problem/detail/pno/(\d+)", page, flags=re.I
            )
        }
    return numbers


def public_problem_page_links(page: str) -> dict[int, str]:
    """Return server-provided pagination links from a public index page."""

    links: dict[int, str] = {}
    for href in re.findall(r"<a\b[^>]*href=[\"']([^\"']+)[\"'][^>]*>", page, flags=re.I | re.S):
        absolute = urljoin(BASE + "/", href)
        parsed = urlparse(absolute)
        if not re.search(r"/index\.php/(?:index/)?problem/(?:index|list)", parsed.path, re.I):
            continue
        match = re.search(r"/(?:p|page)/(\d+)(?:\.html)?$", parsed.path, flags=re.I)
        if not match:
            match = re.search(r"(?:^|[?&])(?:p|page)=(\d+)(?:&|$)", parsed.query, flags=re.I)
        if match:
            links[int(match.group(1))] = absolute
    return links


def all_public_problem_numbers(
    client: YoJClient,
    limiter: RateLimiter,
    max_pages: int,
    contexts: dict[str, list[str]] | None = None,
) -> tuple[set[int], int]:
    """Read the complete public problem index before any submission scan.

    Pagination links are followed exactly as advertised by YOJ.  A small
    sequential fallback covers the older route used by some YOJ deployments
    when the first page does not render numbered links.  An empty result is a
    hard error: treating an authentication or layout failure as "no new
    problems" would incorrectly skip the daily workflow.
    """

    first = fetch(client, limiter, PUBLIC_PROBLEM_INDEX)
    numbers = public_problem_numbers(first)
    if contexts is not None:
        collect_problem_paths(first, contexts, PUBLIC_PROBLEM_INDEX)
        numbers.update(map(int, contexts))
    pages: dict[int, str] = {1: PUBLIC_PROBLEM_INDEX}
    advertised = public_problem_page_links(first)
    if contexts is not None and any(n > max_pages for n in advertised):
        raise RuntimeError("ACCOUNT_PROBLEM_PAGE_LIMIT")
    pages.update({page_no: path for page_no, path in advertised.items() if 1 <= page_no <= max_pages})
    fetched_pages = 1

    if advertised:
        for page_no in sorted(pages):
            if page_no == 1:
                continue
            page = fetch(client, limiter, pages[page_no])
            fetched_pages += 1
            numbers.update(public_problem_numbers(page))
            if contexts is not None:
                collect_problem_paths(page, contexts, pages[page_no])
                numbers.update(map(int, contexts))
            for linked_page, linked_url in public_problem_page_links(page).items():
                if contexts is not None and linked_page > max_pages:
                    raise RuntimeError("ACCOUNT_PROBLEM_PAGE_LIMIT")
                if 1 <= linked_page <= max_pages and linked_page not in pages:
                    pages[linked_page] = linked_url
        # The loop above may discover additional pages while iterating.  Walk
        # until the server-provided pagination graph is exhausted.
        pending = sorted(set(pages) - {1} - set(advertised))
        while pending:
            page_no = pending.pop(0)
            page = fetch(client, limiter, pages[page_no])
            fetched_pages += 1
            numbers.update(public_problem_numbers(page))
            if contexts is not None:
                collect_problem_paths(page, contexts, pages[page_no])
                numbers.update(map(int, contexts))
            for linked_page, linked_url in public_problem_page_links(page).items():
                if contexts is not None and linked_page > max_pages:
                    raise RuntimeError("ACCOUNT_PROBLEM_PAGE_LIMIT")
                if 1 <= linked_page <= max_pages and linked_page not in pages:
                    pages[linked_page] = linked_url
                    pending.append(linked_page)
        if numbers:
            return numbers, fetched_pages

    # No usable pagination links: probe the legacy route until a page has no
    # problem links.  Stop on a repeated page result to avoid looping on a
    # login/error page returned for an invalid page number.
    previous_numbers = set(numbers)
    for page_no in range(2, max_pages + 1):
        page = fetch(client, limiter, PUBLIC_PROBLEM_PAGE.format(page=page_no))
        fetched_pages += 1
        current = public_problem_numbers(page)
        if contexts is not None:
            current_contexts: dict[str, list[str]] = {}
            collect_problem_paths(page, current_contexts, PUBLIC_PROBLEM_PAGE.format(page=page_no))
            current.update(map(int, current_contexts))
            for number, urls in current_contexts.items():
                contexts.setdefault(number, []).extend(u for u in urls if u not in contexts.get(number, []))
        if not current or current == previous_numbers:
            break
        numbers.update(current)
        previous_numbers = current
    else:
        if contexts is not None:
            raise RuntimeError("ACCOUNT_PROBLEM_PAGE_LIMIT")
    if not numbers:
        raise RuntimeError("YOJ 公开题目列表未解析到任何题号；未把本轮误判为无新题")
    return numbers, fetched_pages


def load_public_snapshot(path: Path) -> tuple[set[int], int]:
    """Load a previously captured public-index snapshot without new requests."""

    payload = json.loads(path.read_text(encoding="utf-8"))
    rows = validate_snapshot(payload)
    return {int(row["problemNo"]) for row in rows}, 0


def extract_title(page: str, fallback: str) -> str:
    for pattern in (
        r"<h1[^>]*>(.*?)</h1>",
        r"<h2[^>]*>(.*?)</h2>",
        r"<title[^>]*>(.*?)</title>",
    ):
        match = re.search(pattern, page, flags=re.I | re.S)
        if match:
            title = clean_text(match.group(1))
            if title and "YOJ" not in title.upper():
                return title
    return fallback or "未命名题目"


def load_account_snapshot(path: Path) -> tuple[set[int], int]:
    """Private same-day account evidence must never impersonate a public index."""
    if path.is_symlink():
        raise RuntimeError("账号题库快照不能是符号链接")
    value = json.loads(path.read_text(encoding="utf-8"))
    keys = {"schemaVersion", "scope", "capturedAt", "pagesFetched", "problemNumbers"}
    if isinstance(value, dict) and value.get("schemaVersion") == 2:
        keys |= {"problemPaths", "coverage"}
    if (not isinstance(value, dict) or set(value) != keys
            or type(value["schemaVersion"]) is not int or value["schemaVersion"] not in {1, 2} or value["scope"] != "account_visible"
            or not isinstance(value["capturedAt"], str) or value["capturedAt"][:10] != utc_now()[:10]
            or type(value["pagesFetched"]) is not int or value["pagesFetched"] < 1
            or not isinstance(value["problemNumbers"], list) or not value["problemNumbers"]
            or any(type(n) is not int or n <= 0 for n in value["problemNumbers"])):
        raise RuntimeError("账号题库快照缺失、过期或不受信")
    if value["schemaVersion"] == 2:
        paths = value["problemPaths"]
        coverage = value["coverage"]
        coverage_keys = {"entryPages", "courses", "exams", "homework", "inaccessibleEntries"}
        if (not isinstance(paths, dict) or set(paths) != {str(n) for n in value["problemNumbers"]}
                or not isinstance(coverage, dict) or set(coverage) != coverage_keys
                or any(type(n) is not int or n < 0 for n in coverage.values())
                or coverage["entryPages"] < 2):
            raise RuntimeError("账号题库上下文缺失或不受信")
        for number, urls in paths.items():
            if not isinstance(urls, list) or not urls:
                raise RuntimeError("账号题库上下文缺失或不受信")
            for url in urls:
                if not isinstance(url, str) or contextual_problem_path(int(number), url) != url:
                    raise RuntimeError("账号题库上下文缺失或不受信")
    return set(value["problemNumbers"]), value["pagesFetched"]


def local_problem_numbers(manifest_entries: dict[int, dict[str, Any]]) -> set[int]:
    """Return every problem number that has already appeared locally.

    The manifest is the primary source, while generated data and both public
    problem-directory trees are included as a safety net. A partially
    materialized problem must not be treated as brand-new on the next run.
    """

    numbers = set(manifest_entries)
    if DATA_PROBLEMS_PATH.is_file():
        try:
            payload = json.loads(DATA_PROBLEMS_PATH.read_text(encoding="utf-8"))
        except (OSError, TypeError, ValueError, json.JSONDecodeError):
            payload = {}
        for record in payload.get("records") or []:
            value = record.get("problemNo")
            if str(value).isdigit():
                numbers.add(int(value))
    for problem_root in LOCAL_PROBLEM_ROOTS:
        if not problem_root.is_dir():
            continue
        for path in problem_root.iterdir():
            match = PROBLEM_DIR_RE.match(path.name)
            if match and path.is_dir():
                numbers.add(int(match.group(1)))
    return numbers


def existing_context() -> tuple[dict[str, Any], dict[int, dict[str, Any]], set[int]]:
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    entries = {int(row["problemNo"]): row for row in manifest.get("problems", [])}
    return manifest, entries, local_problem_numbers(entries)


def frozen_public_ready_numbers() -> set[int]:
    if not PUBLIC_READY_PATH.is_file():
        return set()
    payload = json.loads(PUBLIC_READY_PATH.read_text(encoding="utf-8"))
    return {
        int(row["problemNo"])
        for row in payload.get("records") or []
        if row.get("status") == "PUBLIC_READY" and str(row.get("problemNo", "")).isdigit()
    }


def submission_number(value: Any) -> int:
    """Treat topic-only records without a submission as zero."""

    try:
        return int(str(value or "0"))
    except (TypeError, ValueError):
        return 0


class RateLimiter:
    def __init__(self, interval: float) -> None:
        self.interval = max(0.0, interval)
        self.last = 0.0

    def before_request(self) -> None:
        wait_for = self.interval - (time.monotonic() - self.last)
        if wait_for > 0:
            time.sleep(wait_for)
        self.last = time.monotonic()


def fetch(client: YoJClient, limiter: RateLimiter, path: str) -> str:
    limiter.before_request()
    return client.request(path)


def fetch_problem_context(client: YoJClient, limiter: RateLimiter, problem_no: int, paths: list[str]) -> tuple[str, str]:
    for path in paths:
        page = fetch(client, limiter, path)
        if is_access_denied(page):
            continue
        heading = re.search(r'<h1\b[^>]*>(.*?)</h1>', page, re.I | re.S)
        if (not Links(page).complete or not re.search(r'id=[\"\']p_content[\"\']', page)
                or not heading or not re.search(rf'#\s*{problem_no}\s*[.．]', clean_text(heading[1]))):
            raise RuntimeError('ACCOUNT_PROBLEM_RESPONSE_INVALID')
        return path, page
    raise RuntimeError('ACCOUNT_PROBLEM_CONTEXT_UNAVAILABLE')


def all_submission_rows(
    client: YoJClient,
    limiter: RateLimiter,
    max_pages: int,
    stop_after_submission_no: int | None = None,
    require_complete: bool = False,
) -> list[dict[str, Any]]:
    first = fetch(client, limiter, SUBMISSION_INDEX)
    pages = page_numbers(first)
    discovered: dict[int, str] = {1: first}
    first_rows = parse_submission_rows(first)
    if require_complete and not first_rows:
        raise RuntimeError("本人提交列表首页未解析到提交；本轮未将其误判为无新 AC")
    if require_complete and pages and max(pages) > max_pages:
        raise RuntimeError(f"本人提交列表至少有 {max(pages)} 页，超过本轮 {max_pages} 页上限")
    if require_complete:
        previous_numbers = {int(row["submissionNo"]) for row in first_rows}
        for page_no in range(2, max_pages + 1):
            page = fetch(client, limiter, SUBMISSION_PAGE.format(page=page_no))
            page_rows = parse_submission_rows(page)
            current_numbers = {int(row["submissionNo"]) for row in page_rows}
            if not current_numbers or current_numbers == previous_numbers:
                if pages and page_no <= max(pages):
                    raise RuntimeError(f"本人提交列表第 {page_no} 页提前中断或重复；本轮不推断为无新 AC")
                break
            discovered[page_no] = page
            previous_numbers = current_numbers
        else:
            raise RuntimeError(f"本人提交列表达到 {max_pages} 页上限，无法确认候选题的完整 AC 历史")
        accepted: dict[int, dict[str, Any]] = {}
        for page in discovered.values():
            for row in parse_submission_rows(page):
                if str(row.get("status")) == "Accepted":
                    accepted[int(row["submissionNo"])] = row
        return list(accepted.values())
    # Follow server-provided page links first, then probe consecutive pages
    # only while the page still contains rows.  This handles moving totals.
    targets = sorted(set(pages) | set(range(2, (max(pages) if pages else 1) + 1)))
    first_page_is_old = bool(
        stop_after_submission_no is not None
        and first_rows
        and max(int(row["submissionNo"]) for row in first_rows) <= stop_after_submission_no
    )
    if first_page_is_old:
        targets = []
    for page_no in targets:
        if page_no <= 1 or page_no > max_pages:
            continue
        path = SUBMISSION_PAGE.format(page=page_no)
        page = fetch(client, limiter, path)
        discovered[page_no] = page
        page_rows = parse_submission_rows(page)
        if not page_rows and page_no > (max(pages) if pages else 1):
            break
        if stop_after_submission_no is not None and page_rows:
            # YOJ lists submissions newest first.  Once an entire page is at
            # or below the local high-water mark, older pages cannot contain
            # a new submission.  If the ordering ever changes, the page still
            # remains in the discovered set and the caller can use a manual
            # full scan after reviewing the run log.
            if max(int(row["submissionNo"]) for row in page_rows) <= stop_after_submission_no:
                break
    if not pages and not first_page_is_old:
        for page_no in range(2, max_pages + 1):
            path = SUBMISSION_PAGE.format(page=page_no)
            page = fetch(client, limiter, path)
            rows = parse_submission_rows(page)
            if not rows:
                break
            discovered[page_no] = page
            if stop_after_submission_no is not None and max(
                int(row["submissionNo"]) for row in rows
            ) <= stop_after_submission_no:
                break
    rows: dict[int, dict[str, Any]] = {}
    for page in discovered.values():
        for row in parse_submission_rows(page):
            if str(row.get("status")) != "Accepted":
                continue
            submission_no = int(row["submissionNo"])
            if stop_after_submission_no is not None and submission_no <= stop_after_submission_no:
                continue
            previous = rows.get(submission_no)
            if previous is None:
                rows[submission_no] = row
    return list(rows.values())


def save_capture_result(payload: dict[str, Any]) -> None:
    """Persist the scheduler hand-off outside the tracked repository tree."""

    STATE_DIR.mkdir(parents=True, exist_ok=True)
    temporary = CAPTURE_RESULT_PATH.with_name(CAPTURE_RESULT_PATH.name + ".tmp")
    temporary.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    temporary.replace(CAPTURE_RESULT_PATH)


def build_problem_entry(
    row: dict[str, Any],
    title: str,
    folder: str,
    submission_no: int,
    language: str,
    problem_html: str,
    detail_html: str,
    code: bytes,
    response: dict[str, Any],
    now: str,
) -> dict[str, Any]:
    problem_no = int(row["problemNo"])
    padded = f"{problem_no:04d}"
    extension = language_extension(language)
    code_name = f"{padded}_提交_{submission_no}_{language}_"
    return {
        "problemNo": str(problem_no),
        "title": title,
        "folder": folder,
        "submissionNo": str(submission_no),
        "language": language,
        "files": {
            "problem": f"{folder}/{padded}_题目原文.html",
            "submission": f"{folder}/{padded}_提交_{submission_no}_详情.html",
            "completeCode": f"{folder}/{code_name}完整代码{extension}",
            "directlySubmittableCode": f"{folder}/{code_name}可提交代码{extension}",
            "metadata": f"{folder}/{padded}_元数据.json",
        },
        "codeSha256": digest_bytes(code),
        "_materialized": {
            "problemHtml": problem_html,
            "detailHtml": detail_html,
            "code": code,
            "response": response,
            "capturedAt": now,
        },
    }


def build_topic_entry(
    problem_no: int,
    title: str,
    folder: str,
    problem_html: str,
    now: str,
) -> dict[str, Any]:
    """Build a topic-only entry when no local Accepted source is available."""

    padded = f"{problem_no:04d}"
    return {
        "problemNo": str(problem_no),
        "title": title,
        "folder": folder,
        "submissionNo": "",
        "language": "",
        "status": "TOPIC_CAPTURED",
        "files": {
            "problem": f"{folder}/{padded}_题目原文.html",
            "metadata": f"{folder}/{padded}_元数据.json",
        },
        "codeSha256": "",
        "_materialized": {
            "problemHtml": problem_html,
            "capturedAt": now,
        },
    }


def materialize(entry: dict[str, Any]) -> int:
    materialized = entry.pop("_materialized")
    problem_no = int(entry["problemNo"])
    folder = ROOT / "代码库" / entry["folder"]
    files = entry["files"]
    code = materialized["code"]
    changed = 0
    changed += int(atomic_write(folder / Path(files["problem"]).name, materialized["problemHtml"].encode("utf-8")))
    changed += int(atomic_write(folder / Path(files["submission"]).name, materialized["detailHtml"].encode("utf-8")))
    changed += int(atomic_write(folder / Path(files["completeCode"]).name, code))
    changed += int(atomic_write(folder / Path(files["directlySubmittableCode"]).name, code))
    metadata = {
        "problem": {
            "submissionNo": entry["submissionNo"],
            "problemNo": entry["problemNo"],
            "title": entry["title"],
            "status": "Accepted",
            "score": "100",
            "language": entry["language"],
            "submitter": "本人(已脱敏)",
            "submissionUrl": f"{BASE}/index.php/index/submissions/detail/subno/{entry['submissionNo']}.html",
            "problemUrl": BASE + contextual_problem_path(problem_no, entry.get("problemUrl")),
        },
        "submission": {
            "submissionNo": entry["submissionNo"],
            "problemNo": entry["problemNo"],
            "title": entry["title"],
            "status": "Accepted",
            "score": "100",
            "language": entry["language"],
            "submitter": "本人(已脱敏)",
            "submissionUrl": f"{BASE}/index.php/index/submissions/detail/subno/{entry['submissionNo']}.html",
            "problemUrl": BASE + contextual_problem_path(problem_no, entry.get("problemUrl")),
        },
        "source": {
            "captureVersion": 2,
            "capturedAt": materialized["capturedAt"],
            "site": BASE,
            "codeEndpoint": "/index.php/index/submissions/getcode.html",
            "endpointResponse": materialized["response"],
        },
        "files": {key: Path(value).name for key, value in files.items()},
        "code": {
            "language": entry["language"],
            "extension": Path(files["completeCode"]).suffix,
            "bytesUtf8": len(code),
            "sha256": entry["codeSha256"],
            "blockHandling": {
                "status": "endpoint_returned_one_code_field",
                "completeCodeFile": Path(files["completeCode"]).name,
                "directlySubmittableFile": Path(files["directlySubmittableCode"]).name,
                "blocks": [],
                "note": "getcode 接口返回一个 code 字段；未猜测多框边界。",
            },
        },
    }
    metadata_path = folder / Path(files["metadata"]).name
    changed += int(atomic_write(metadata_path, (json.dumps(metadata, ensure_ascii=False, indent=2) + "\n").encode("utf-8")))
    return changed


def materialize_topic(entry: dict[str, Any]) -> int:
    materialized = entry.pop("_materialized")
    problem_no = int(entry["problemNo"])
    folder = ROOT / "代码库" / entry["folder"]
    files = entry["files"]
    changed = 0
    changed += int(atomic_write(folder / Path(files["problem"]).name, materialized["problemHtml"].encode("utf-8")))
    metadata = {
        "problem": {
            "submissionNo": "",
            "problemNo": entry["problemNo"],
            "title": entry["title"],
            "status": "TOPIC_CAPTURED",
            "score": "",
            "language": "",
            "submitter": "未抓取源码",
            "submissionUrl": "",
            "problemUrl": BASE + contextual_problem_path(problem_no, entry.get("problemUrl")),
        },
        "source": {
            "captureVersion": 3,
            "capturedAt": materialized["capturedAt"],
            "site": BASE,
            "discovery": "public_problem_index",
            "note": "公开题目列表首次发现；本轮未找到本人 Accepted 源码，未生成或伪造代码文件。",
        },
        "files": {
            "problemHtml": Path(files["problem"]).name,
            "metadata": Path(files["metadata"]).name,
        },
        "code": {
            "status": "NO_LOCAL_AC",
            "language": "",
            "extension": "",
            "bytesUtf8": 0,
            "sha256": "",
        },
    }
    metadata_path = folder / Path(files["metadata"]).name
    changed += int(atomic_write(metadata_path, (json.dumps(metadata, ensure_ascii=False, indent=2) + "\n").encode("utf-8")))
    return changed


def main() -> int:
    parser = argparse.ArgumentParser(description="读取 YOJ 新题面和指定待复验题目的本人 Accepted 源码")
    parser.add_argument("--request-interval", type=float, default=1.0, help="只读请求最小间隔秒数，默认 1")
    parser.add_argument("--max-pages", type=int, default=250, help="公开题目/提交列表最多扫描页数，默认 250")
    parser.add_argument(
        "--public-only",
        action="store_true",
        help="不登录、不扫描提交，只为公开新题号抓取题面并写入 TOPIC_CAPTURED",
    )
    parser.add_argument(
        "--discover-only",
        action="store_true",
        help="只发现题号，不抓题面或扫描提交；--account-visible 时先登录",
    )
    parser.add_argument(
        "--public-snapshot",
        type=Path,
        help="使用已保存的公开列表快照计算差集，避免再次请求 YOJ 公开列表",
    )
    parser.add_argument("--account-visible", action="store_true", help="登录后发现账号可见题目；账号快照仅保存在本机")
    parser.add_argument("--account-snapshot", type=Path, help="抓取阶段复用本轮账号题号快照；仍需登录")
    parser.add_argument(
        "--ac-target",
        dest="ac_targets",
        action="append",
        type=int,
        help="检查所选可见范围内尚未发布题目的本人新 Accepted；可重复指定",
    )
    args = parser.parse_args()
    if args.request_interval < 0 or args.max_pages < 1:
        raise SystemExit("请求间隔必须非负，页数必须为正数")
    if args.public_only and args.discover_only:
        raise SystemExit("--public-only 不能与 --discover-only 同用")
    if (args.account_visible and (args.public_only or args.public_snapshot)) or (args.account_snapshot and not args.account_visible):
        raise SystemExit("账号发现与匿名公开快照必须分离")
    ac_targets = sorted(set(args.ac_targets or []))
    if ac_targets and (args.public_only or args.discover_only):
        raise SystemExit("--ac-target 仅可用于已登录的源码抓取阶段")
    if any(problem_no <= 0 for problem_no in ac_targets):
        raise SystemExit("--ac-target 题号必须为正整数")

    manifest, existing, known_problem_numbers = existing_context()
    client = YoJClient()
    limiter = RateLimiter(args.request_interval)
    now = utc_now()
    high_water = max((submission_number(row.get("submissionNo")) for row in existing.values()), default=0)
    logged_in = False
    problem_paths: dict[str, list[str]] = {}
    number_scope = "account_visible_problem_numbers" if args.account_visible else "public_problem_numbers"
    count_key = "accountVisibleProblemNumbersSeen" if args.account_visible else "publicProblemNumbersSeen"
    pages_key = "accountVisibleProblemListPages" if args.account_visible else "publicProblemListPages"
    if args.account_visible:
        save_capture_result({"schemaVersion": 2, "scope": "account_login_pending", "capturedAt": now})
        try:
            client.login()
        except Exception as exc:
            reason = "YOJ_AUTH_REQUIRED" if isinstance(exc, RuntimeError) else classify_yoj_error(exc)
            save_capture_result({"schemaVersion": 2, "scope": "account_login_unavailable", "capturedAt": now,
                                 "authenticated": False, "reason": reason})
            raise
        logged_in = True
        save_capture_result({"schemaVersion": 2, "scope": "account_list_pending", "capturedAt": now})
        try:
            if args.account_snapshot and not args.discover_only:
                public_numbers, public_pages = load_account_snapshot(args.account_snapshot)
                saved = json.loads(args.account_snapshot.read_text(encoding="utf-8"))
                if saved["schemaVersion"] != 2:
                    raise RuntimeError("账号发现快照未覆盖课程和考试；必须重新发现")
                problem_paths = saved["problemPaths"]
            else:
                public_numbers, public_pages = all_public_problem_numbers(client, limiter, args.max_pages, contexts=problem_paths)
                problem_paths, coverage = discover_account_contexts(client, limiter, public_numbers, args.max_pages, initial_paths=problem_paths)
                public_numbers = {int(n) for n in problem_paths}
                public_pages += coverage["entryPages"]
                snapshot = {"schemaVersion": 2, "scope": "account_visible", "capturedAt": now,
                            "pagesFetched": public_pages, "problemNumbers": sorted(public_numbers),
                            "problemPaths": problem_paths, "coverage": coverage}
                atomic_write(STATE_DIR / "account-visible-problems.json", (json.dumps(snapshot) + "\n").encode())
        except Exception as exc:
            # Login succeeded, but an incomplete graph is not a new snapshot.
            # Store only a fixed reason, never account URLs or response text.
            save_capture_result({"schemaVersion": 2, "scope": "account_list_unavailable", "capturedAt": now,
                                 "authenticated": True, "reason": classify_yoj_error(exc)})
            raise
    elif args.public_snapshot:
        public_numbers, public_pages = load_public_snapshot(args.public_snapshot)
    else:
        public_numbers, public_pages = all_public_problem_numbers(client, limiter, args.max_pages)
    new_problem_numbers = sorted(public_numbers - known_problem_numbers)
    if ac_targets:
        unknown = set(ac_targets) - set(existing)
        unavailable = set(ac_targets) - public_numbers
        frozen = set(ac_targets) & frozen_public_ready_numbers()
        if unknown or unavailable or frozen:
            raise RuntimeError(
                "AC 目标与当前题库/公开快照/冻结发布记录不一致："
                f"未归档 {sorted(unknown)}，未公开 {sorted(unavailable)}，已发布 {sorted(frozen)}"
            )
    if not new_problem_numbers and not ac_targets:
        result = {
            "schemaVersion": 2,
            "status": "NO_NEW_PROBLEMS",
            "scope": number_scope + "_only",
            "capturedAt": now,
            pages_key: public_pages,
            count_key: len(public_numbers),
            "highWaterSubmissionNo": high_water,
            "acceptedProblemsSeen": None,
            "submissionScan": "SKIPPED_NO_NEW_PROBLEMS",
            "knownProblemNumbers": len(known_problem_numbers),
            "newProblemNumbers": [],
            "newTopicNumbers": [],
            "newAcNumbers": [],
            "changedProblems": 0,
            "changedFiles": 0,
            "manifestUpdated": False,
            "downstream": "SKIP_ALL",
        }
        save_capture_result(result)
        print(json.dumps(result, ensure_ascii=False))
        return 0

    if args.discover_only:
        result = {
            "schemaVersion": 2,
            "status": "NEW_PROBLEMS_FOUND",
            "scope": number_scope + "_only",
            "capturedAt": now,
            pages_key: public_pages,
            count_key: len(public_numbers),
            "highWaterSubmissionNo": high_water,
            "acceptedProblemsSeen": None,
            "submissionScan": "SKIPPED_DISCOVERY_ONLY",
            "knownProblemNumbers": len(known_problem_numbers),
            "newProblemNumbers": new_problem_numbers,
            "newTopicNumbers": [],
            "newAcNumbers": [],
            "changedProblems": 0,
            "changedFiles": 0,
            "manifestUpdated": False,
            "downstream": "DISCOVERY_ONLY",
        }
        save_capture_result(result)
        print(json.dumps(result, ensure_ascii=False))
        return 0

    if not args.public_only and not logged_in:
        # The authenticated path runs for a new public topic or an explicit
        # pending-AC target. Published problems are rejected above.
        client.login()

    if args.public_only:
        changed_files = 0
        changed_problems = 0
        for pno in new_problem_numbers:
            problem_path = f"/index.php/index/problem/detail/pno/{pno}.html"
            problem_html = fetch(client, limiter, problem_path)
            title = extract_title(problem_html, "")
            folder_name = f"{pno:04d}_{safe_title(title)}"
            candidate = build_topic_entry(pno, title, folder_name, problem_html, now)
            changed_files += materialize_topic(candidate)
            candidate.pop("_materialized", None)
            manifest_entry = {key: value for key, value in candidate.items() if not key.startswith("_")}
            manifest.setdefault("problems", []).append(manifest_entry)
            existing[pno] = manifest_entry
            changed_problems += 1

        manifest["problems"] = sorted(manifest.get("problems", []), key=lambda item: int(item["problemNo"]))
        manifest["capturedAt"] = now
        totals = manifest.setdefault("totals", {})
        totals["uniqueProblems"] = len(manifest["problems"])
        totals["materializedProblemDirectories"] = len(manifest["problems"])
        totals["materializedCodeFiles"] = sum(
            bool((item.get("files") or {}).get("completeCode")) for item in manifest["problems"]
        )
        atomic_write(MANIFEST_PATH, (json.dumps(manifest, ensure_ascii=False, indent=2) + "\n").encode("utf-8"))
        result = {
            "schemaVersion": 2,
            "status": "NEW_PROBLEMS_FOUND",
            "scope": "public_problem_numbers_only",
            "capturedAt": now,
            "publicProblemListPages": public_pages,
            "publicProblemNumbersSeen": len(public_numbers),
            "highWaterSubmissionNo": high_water,
            "acceptedProblemsSeen": None,
            "submissionScan": "SKIPPED_PUBLIC_ONLY",
            "knownProblemNumbers": len(known_problem_numbers),
            "newProblemNumbers": new_problem_numbers,
            "newTopicNumbers": new_problem_numbers,
            "newAcNumbers": [],
            "changedProblems": changed_problems,
            "changedFiles": changed_files,
            "manifestUpdated": bool(changed_files or changed_problems),
            "downstream": "TOPIC_CAPTURED_ONLY",
        }
        save_capture_result(result)
        print(json.dumps(result, ensure_ascii=False))
        return 0

    # Scan the full personal history for the selected pending topics. Their
    # first Accepted can predate the latest submission already in the archive.
    accepted_rows = all_submission_rows(
        client, limiter, args.max_pages, None,
        require_complete=bool(ac_targets or (args.account_visible and new_problem_numbers)),
    )
    latest: dict[int, dict[str, Any]] = {}
    for row in accepted_rows:
        pno = int(row["problemNo"])
        previous = latest.get(pno)
        if previous is None or int(row["submissionNo"]) > int(previous["submissionNo"]):
            latest[pno] = row

    changed_files = 0
    changed_problems = 0
    new_topic_numbers: list[int] = []
    new_ac_numbers: list[int] = []
    refreshed_ac_numbers: list[int] = []
    context_updated_numbers: list[int] = []
    unavailable_existing_numbers: list[int] = []
    manifest_entries = {int(row["problemNo"]): row for row in manifest.get("problems") or []}
    for pno in sorted(set(new_problem_numbers) | set(ac_targets)):
        previous_entry = existing.get(pno)
        row = latest.get(pno)
        if previous_entry is not None and (
            row is None or int(row["submissionNo"]) <= submission_number(previous_entry.get("submissionNo"))
        ):
            # Refresh routing for an unpublished archive independently from
            # source replacement. Never recapture/replace unchanged AC bytes.
            if args.account_visible:
                files = previous_entry.get("files") or {}
                metadata_path = ROOT / "代码库" / str(files.get("metadata") or "")
                metadata = json.loads(metadata_path.read_text(encoding="utf-8")) if metadata_path.is_file() else {}
                previous_url = previous_entry.get("problemUrl") or (metadata.get("problem") or {}).get("problemUrl")
                previous_path = contextual_problem_path(pno, previous_url)
                paths = problem_paths[str(pno)]
                ordered_paths = ([previous_path] if previous_path in paths else []) + [p for p in paths if p != previous_path]
                try:
                    problem_path, problem_html = fetch_problem_context(client, limiter, pno, ordered_paths)
                except RuntimeError as exc:
                    if str(exc) != 'ACCOUNT_PROBLEM_CONTEXT_UNAVAILABLE':
                        raise
                    # A complete, explicit access denial for an OLD archive
                    # is a per-problem block, not evidence of a failed login
                    # or permission to overwrite the archive. New topics and
                    # malformed/network responses still fail closed.
                    unavailable_existing_numbers.append(pno)
                    continue
                if problem_path != previous_path:
                    if not metadata_path.is_file() or not files.get("problem"):
                        raise RuntimeError("ACCOUNT_PROBLEM_METADATA_MISSING")
                    problem_url = BASE + contextual_problem_path(pno, problem_path)
                    for section in ("problem", "submission"):
                        if section in metadata:
                            metadata[section]["problemUrl"] = problem_url
                    changed_files += int(atomic_write(metadata_path, (json.dumps(metadata, ensure_ascii=False, indent=2) + "\n").encode()))
                    changed_files += int(atomic_write(ROOT / "代码库" / files["problem"], problem_html.encode()))
                    updated = dict(previous_entry, problemUrl=problem_url)
                    manifest_entries[pno] = updated
                    existing[pno] = updated
                    context_updated_numbers.append(pno)
                    changed_problems += 1
            continue
        paths = problem_paths.get(str(pno), [contextual_problem_path(pno)])
        problem_path, problem_html = fetch_problem_context(client, limiter, pno, paths)
        title = str((previous_entry or {}).get("title") or extract_title(problem_html, ""))
        folder_name = str((previous_entry or {}).get("folder") or f"{pno:04d}_{safe_title(title)}")

        if row is None:
            candidate = build_topic_entry(pno, title, folder_name, problem_html, now)
            candidate["problemUrl"] = BASE + contextual_problem_path(pno, problem_path)
            changed_files += materialize_topic(candidate)
            candidate.pop("_materialized", None)
            manifest_entry = {key: value for key, value in candidate.items() if not key.startswith("_")}
            manifest_entries[pno] = manifest_entry
            existing[pno] = manifest_entry
            new_topic_numbers.append(pno)
            changed_problems += 1
            continue

        submission_no = int(row["submissionNo"])
        detail_path = f"/index.php/index/submissions/detail/subno/{submission_no}.html"
        detail_html = fetch(client, limiter, detail_path)
        limiter.before_request()
        code_raw = client.request(
            "/index.php/index/submissions/getcode.html",
            {"subid": str(submission_no)},
            detail_path,
        )
        payload = json.loads(code_raw.lstrip("\ufeff"))
        if str(payload.get("status")) != "1" or not isinstance(payload.get("code"), str):
            raise RuntimeError(f"提交 {submission_no} 的 getcode 没有返回有效源码")
        code = payload["code"].encode("utf-8")
        candidate = build_problem_entry(
            row,
            title,
            folder_name,
            submission_no,
            str(row.get("language") or "unknown"),
            problem_html,
            detail_html,
            code,
            {"status": payload.get("status"), "tkhide": payload.get("tkhide")},
            now,
        )
        candidate["problemUrl"] = BASE + contextual_problem_path(pno, problem_path)
        changed_files += materialize(candidate)
        candidate.pop("_materialized", None)
        manifest_entry = {key: value for key, value in candidate.items() if not key.startswith("_")}
        manifest_entries[pno] = manifest_entry
        existing[pno] = manifest_entry
        new_ac_numbers.append(pno)
        if pno in ac_targets:
            refreshed_ac_numbers.append(pno)
        changed_problems += 1

    if changed_files or changed_problems:
        manifest["problems"] = [manifest_entries[key] for key in sorted(manifest_entries)]
        manifest["capturedAt"] = now
        totals = manifest.setdefault("totals", {})
        totals["uniqueProblems"] = len(manifest["problems"])
        totals["materializedProblemDirectories"] = len(manifest["problems"])
        totals["materializedCodeFiles"] = sum(
            bool((item.get("files") or {}).get("completeCode")) for item in manifest["problems"]
        )
        atomic_write(MANIFEST_PATH, (json.dumps(manifest, ensure_ascii=False, indent=2) + "\n").encode("utf-8"))
    result = {
        "schemaVersion": 2,
        "status": "NEW_PROBLEMS_FOUND" if new_problem_numbers else "NEW_AC_FOUND" if refreshed_ac_numbers else "CONTEXT_UPDATED" if context_updated_numbers else "NO_NEW_PROBLEMS",
        "scope": number_scope + "_and_personal_accepted",
        "capturedAt": now,
        pages_key: public_pages,
        count_key: len(public_numbers),
        "highWaterSubmissionNo": high_water,
        "acceptedProblemsSeen": len(latest),
        "submissionScan": "FULL_TARGETED" if ac_targets else "FULL_ON_NEW_TOPIC",
        "knownProblemNumbers": len(known_problem_numbers),
        "acTargetNumbers": ac_targets,
        "newProblemNumbers": new_problem_numbers,
        "newTopicNumbers": new_topic_numbers,
        "newAcNumbers": new_ac_numbers,
        "refreshedAcNumbers": refreshed_ac_numbers,
        "contextUpdatedNumbers": context_updated_numbers,
        "unavailableProblemNumbers": unavailable_existing_numbers,
        "changedProblems": changed_problems,
        "changedFiles": changed_files,
        "manifestUpdated": bool(changed_files or changed_problems),
        "downstream": "SELECTED_PROBLEM_NUMBERS_ONLY" if new_problem_numbers or refreshed_ac_numbers or context_updated_numbers else "SKIP_ALL",
    }
    save_capture_result(result)
    print(
        json.dumps(result, ensure_ascii=False)
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
