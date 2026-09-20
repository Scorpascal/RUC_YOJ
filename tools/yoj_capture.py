#!/usr/bin/env python3
"""Incrementally capture YOJ problem pages and the user's latest AC source.

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
from urllib.parse import urljoin

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))
from online_verify import YoJClient, clean_text, parse_submission_rows  # noqa: E402


MANIFEST_PATH = ROOT / "代码库" / "AC抓取清单.json"
BASE = "http://yoj.ruc.edu.cn"
SUBMISSION_INDEX = "/index.php/index/submissions/index.html"
SUBMISSION_PAGE = "/index.php/submissions/index/p/{page}.html"


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


def existing_context() -> tuple[dict[str, Any], dict[int, dict[str, Any]]]:
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    entries = {int(row["problemNo"]): row for row in manifest.get("problems", [])}
    return manifest, entries


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


def all_submission_rows(client: YoJClient, limiter: RateLimiter, max_pages: int) -> list[dict[str, Any]]:
    first = fetch(client, limiter, SUBMISSION_INDEX)
    pages = page_numbers(first)
    discovered: dict[int, str] = {1: first}
    # Follow server-provided page links first, then probe consecutive pages
    # only while the page still contains rows.  This handles moving totals.
    targets = sorted(set(pages) | set(range(2, (max(pages) if pages else 1) + 1)))
    for page_no in targets:
        if page_no <= 1 or page_no > max_pages:
            continue
        path = SUBMISSION_PAGE.format(page=page_no)
        page = fetch(client, limiter, path)
        discovered[page_no] = page
        if not parse_submission_rows(page) and page_no > (max(pages) if pages else 1):
            break
    if not pages:
        for page_no in range(2, max_pages + 1):
            path = SUBMISSION_PAGE.format(page=page_no)
            page = fetch(client, limiter, path)
            rows = parse_submission_rows(page)
            if not rows:
                break
            discovered[page_no] = page
    rows: dict[int, dict[str, Any]] = {}
    for page in discovered.values():
        for row in parse_submission_rows(page):
            if str(row.get("status")) != "Accepted":
                continue
            submission_no = int(row["submissionNo"])
            previous = rows.get(submission_no)
            if previous is None:
                rows[submission_no] = row
    return list(rows.values())


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
            "problemUrl": f"{BASE}/index.php/index/problem/detail/pno/{problem_no}.html",
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
            "problemUrl": f"{BASE}/index.php/index/problem/detail/pno/{problem_no}.html",
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


def main() -> int:
    parser = argparse.ArgumentParser(description="增量读取 YOJ 题面和本人 Accepted 源码")
    parser.add_argument("--request-interval", type=float, default=1.0, help="只读请求最小间隔秒数，默认 1")
    parser.add_argument("--max-pages", type=int, default=100, help="提交列表最多扫描页数，默认 100")
    args = parser.parse_args()
    if args.request_interval < 0 or args.max_pages < 1:
        raise SystemExit("请求间隔必须非负，页数必须为正数")

    manifest, existing = existing_context()
    client = YoJClient()
    client.login()
    limiter = RateLimiter(args.request_interval)
    now = utc_now()
    accepted_rows = all_submission_rows(client, limiter, args.max_pages)
    latest: dict[int, dict[str, Any]] = {}
    for row in accepted_rows:
        pno = int(row["problemNo"])
        previous = latest.get(pno)
        if previous is None or int(row["submissionNo"]) > int(previous["submissionNo"]):
            latest[pno] = row

    changed_files = 0
    changed_problems = 0
    for pno, row in sorted(latest.items()):
        old = existing.get(pno)
        title_fallback = str(old.get("title") if old else "").strip()
        problem_path = f"/index.php/index/problem/detail/pno/{pno}.html"
        problem_html = fetch(client, limiter, problem_path)
        title = extract_title(problem_html, title_fallback)
        if old:
            folder_name = str(old["folder"])
        else:
            folder_name = f"{pno:04d}_{safe_title(title)}"

        if old and int(old.get("submissionNo", 0)) >= int(row["submissionNo"]):
            # A page can change without a new AC.  Refresh the stable problem
            # snapshot, then leave the prior source selection untouched.
            target = ROOT / "代码库" / folder_name / f"{pno:04d}_题目原文.html"
            changed_files += int(atomic_write(target, problem_html.encode("utf-8")))
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
            str(row.get("language") or old.get("language") if old else row.get("language") or "unknown"),
            problem_html,
            detail_html,
            code,
            {"status": payload.get("status"), "tkhide": payload.get("tkhide")},
            now,
        )
        changed_files += materialize(candidate)
        candidate.pop("_materialized", None)
        manifest_entry = {key: value for key, value in candidate.items() if not key.startswith("_")}
        if old:
            existing[pno] = manifest_entry
            changed_problems += 1
        else:
            manifest.setdefault("problems", []).append(manifest_entry)
            existing[pno] = manifest_entry
            changed_problems += 1

    if changed_files or changed_problems:
        manifest["problems"] = sorted(manifest.get("problems", []), key=lambda item: int(item["problemNo"]))
        manifest["capturedAt"] = now
        totals = manifest.setdefault("totals", {})
        totals["uniqueProblems"] = len(manifest["problems"])
        totals["materializedProblemDirectories"] = len(manifest["problems"])
        totals["materializedCodeFiles"] = sum(
            bool((item.get("files") or {}).get("completeCode")) for item in manifest["problems"]
        )
        atomic_write(MANIFEST_PATH, (json.dumps(manifest, ensure_ascii=False, indent=2) + "\n").encode("utf-8"))
    print(
        json.dumps(
            {
                "acceptedProblemsSeen": len(latest),
                "changedProblems": changed_problems,
                "changedFiles": changed_files,
                "manifestUpdated": bool(changed_files or changed_problems),
            },
            ensure_ascii=False,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
