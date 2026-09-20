#!/usr/bin/env python3
"""One-shot, resumable YOJ online verification runner.

Credentials are read from the process environment and never written to disk.
The runner intentionally uses one submission at a time and persists only
submission evidence in the ignored staging report.
"""

from __future__ import annotations

import hashlib
import html
import json
import os
import re
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any
from urllib.parse import urlencode, urljoin
from urllib.request import HTTPCookieProcessor, Request, build_opener
from http.cookiejar import CookieJar


ROOT = Path(os.environ["YOJ_ROOT"]).resolve()
BASE = "http://yoj.ruc.edu.cn"
LOGIN_USER = os.environ["YOJ_LOGIN_USER"]
LOGIN_PASS = os.environ["YOJ_LOGIN_PASS"]
REPORT_PATH = ROOT / "staging" / "online-verification.json"
SUBMIT_INTERVAL = 15.0
POLL_INTERVAL = 3.0
POLL_LIMIT = 30
BUILD_EVERY = 20
FILL_IN_PROBLEMS = {285, 286}
PENDING_STATUSES = {"waiting", "compiling", "running", "judging", "pending"}


def utc_now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def clean_text(value: str) -> str:
    value = html.unescape(re.sub(r"<[^>]*>", " ", value))
    return re.sub(r"\s+", " ", value).strip()


class YoJClient:
    def __init__(self) -> None:
        self.opener = build_opener(HTTPCookieProcessor(CookieJar()))

    def request(self, path: str, data: dict[str, str] | None = None, referer: str = "") -> str:
        url = urljoin(BASE + "/", path)
        body = urlencode(data or {}).encode("utf-8") if data is not None else None
        headers = {
            "User-Agent": "Mozilla/5.0 (Macintosh; Intel Mac OS X) YOJ-verification-runner",
            "Accept": "text/html,application/json;q=0.9,*/*;q=0.8",
        }
        if referer:
            headers["Referer"] = urljoin(BASE + "/", referer)
        if data is not None:
            headers["Content-Type"] = "application/x-www-form-urlencoded"
            headers["X-Requested-With"] = "XMLHttpRequest"
        request = Request(url, data=body, headers=headers, method="POST" if data is not None else "GET")
        with self.opener.open(request, timeout=35) as response:
            return response.read().decode("utf-8", errors="replace")

    def login(self) -> None:
        raw = self.request(
            "/index.php/index/login/login2.html",
            {"username": LOGIN_USER, "passwd": LOGIN_PASS},
            "/index.php/index/index/signin.html",
        ).lstrip("\ufeff")
        result = json.loads(raw)
        if str(result.get("status")) != "1":
            raise RuntimeError(f"YOJ 登录失败: {result}")


def candidate_info(path: Path) -> tuple[int, str] | None:
    match = re.match(r"^(\d{4})_提交_\d+_(.+)_可提交代码$", path.stem)
    if not match:
        return None
    return int(match.group(1)), match.group(2)


def load_context() -> tuple[dict[int, dict[str, Any]], dict[int, dict[str, Any]], dict[int, dict[str, Any]]]:
    data = json.loads((ROOT / "data" / "problems.json").read_text(encoding="utf-8"))
    problems = {int(row["problemNo"]): row for row in data.get("records", [])}
    report = json.loads(REPORT_PATH.read_text(encoding="utf-8"))
    records = {int(row["problemNo"]): row for row in report.get("records", []) if row.get("problemNo") is not None}
    skips = {int(row["problemNo"]): row for row in report.get("skipped", []) if row.get("problemNo") is not None}
    return problems, records, skips


def save_report(records: dict[int, dict[str, Any]], skips: dict[int, dict[str, Any]]) -> None:
    report = json.loads(REPORT_PATH.read_text(encoding="utf-8"))
    report["updatedAt"] = utc_now()
    report["records"] = [records[key] for key in sorted(records)]
    report["skipped"] = [skips[key] for key in sorted(skips)]
    temp_path = REPORT_PATH.with_name(REPORT_PATH.name + ".tmp")
    temp_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    temp_path.replace(REPORT_PATH)


def read_candidates() -> dict[int, tuple[Path, str]]:
    candidates: dict[int, tuple[Path, str]] = {}
    for path in sorted((ROOT / "staging" / "cleaned-code").glob("????_*/*_可提交代码.*"), key=lambda item: item.as_posix()):
        info = candidate_info(path)
        if info is None:
            continue
        problem_no, language = info
        if problem_no in candidates:
            raise RuntimeError(f"同一题发现多个可提交候选: {problem_no}")
        candidates[problem_no] = (path, language)
    return candidates


def parse_form(page: str, problem_no: int, language: str) -> tuple[str, str | None]:
    form_tags = re.findall(r"<form\b[^>]*>", page, flags=re.I)
    submit_tag = next((tag for tag in form_tags if re.search(r"id=[\"']submit_code[\"']", tag, re.I)), "")
    if not submit_tag:
        raise ValueError("SUBMIT_FORM_NOT_FOUND")
    action_match = re.search(r"action=[\"']([^\"']+)[\"']", submit_tag, re.I)
    action = action_match.group(1) if action_match else "/index.php/index/index/prob_submit.html"
    pid_match = re.search(r"name=[\"']pid[\"'][^>]*value=[\"']([^\"']+)[\"']", page, re.I)
    if not pid_match:
        pid_match = re.search(r"value=[\"']([^\"']+)[\"'][^>]*name=[\"']pid[\"']", page, re.I)
    if not pid_match or int(pid_match.group(1)) != problem_no:
        raise ValueError("PROBLEM_ID_MISMATCH")
    if len(re.findall(r"id=[\"']editor[\"']", page, flags=re.I)) != 1:
        raise ValueError("SPECIAL_OR_MULTIPLE_EDITOR")
    if not re.search(r"name=[\"']code[\"']", page, flags=re.I):
        raise ValueError("CODE_FIELD_NOT_FOUND")
    values = set(re.findall(r"data-value=[\"']([^\"']+)[\"']", page, flags=re.I))
    if language not in values:
        raise ValueError(f"LANGUAGE_NOT_AVAILABLE:{language}")
    return urljoin(BASE + "/", action), pid_match.group(1)


def parse_submission_rows(page: str) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for row_html in re.findall(r"<tr\b[^>]*>(.*?)</tr>", page, flags=re.I | re.S):
        sub_match = re.search(r"/submissions/detail/subno/(\d+)", row_html, re.I)
        pno_match = re.search(r"/problem/detail/pno/(\d+)", row_html, re.I)
        if not sub_match or not pno_match:
            continue
        status_match = re.search(r"class=[\"'][^\"']*\bstatus\b[^\"']*[\"'][^>]*>(.*?)</span>", row_html, re.I | re.S)
        score_match = re.search(r"class=[\"'][^\"']*\bscore\b[^\"']*[\"'][^>]*>(.*?)</span>", row_html, re.I | re.S)
        code_match = re.search(r"<a\b[^>]*>([^<]+)</a>\s*/\s*(\d+)\s*B", row_html, re.I | re.S)
        plain = clean_text(row_html)
        time_match = re.search(r"(\d+)\s*ms\b", plain)
        memory_match = re.search(r"(\d+)\s*KB\b", plain)
        rows.append(
            {
                "submissionNo": int(sub_match.group(1)),
                "problemNo": int(pno_match.group(1)),
                "status": clean_text(status_match.group(1)) if status_match else "UNKNOWN",
                "score": int(clean_text(score_match.group(1))) if score_match and clean_text(score_match.group(1)).isdigit() else 0,
                "timeMs": int(time_match.group(1)) if time_match else 0,
                "memoryKiB": int(memory_match.group(1)) if memory_match else 0,
                "language": clean_text(code_match.group(1)) if code_match else "",
                "submittedBytes": int(code_match.group(2)) if code_match else 0,
            }
        )
    return rows


def fetch_rows(client: YoJClient) -> list[dict[str, Any]]:
    return parse_submission_rows(client.request("/index.php/index/submissions/index.html"))


def recover_source(client: YoJClient, submission_no: int, candidate_bytes: bytes) -> dict[str, Any]:
    try:
        raw = client.request(
            "/index.php/index/submissions/getcode.html",
            {"subid": str(submission_no)},
            f"/index.php/index/submissions/detail/subno/{submission_no}.html",
        ).lstrip("\ufeff")
        payload = json.loads(raw)
        source = payload.get("code") if str(payload.get("status")) == "1" else None
        if not isinstance(source, str):
            raise ValueError("源码接口没有返回 code")
        source_bytes = source.encode("utf-8")
        exact = source_bytes == candidate_bytes
        return {
            "status": "VISIBLE",
            "sourceVisibleInSubmissionDetail": True,
            "exactByteMatch": exact,
            "sourceSha256": hashlib.sha256(source_bytes).hexdigest(),
            "matchNote": "终端 getcode 接口回收源码并完成候选字节比对。" if exact else "终端 getcode 接口回收源码，但与本地候选字节不完全一致。",
        }
    except Exception as exc:  # noqa: BLE001 - evidence is retained as a review state
        return {
            "status": "PENDING_DETAIL",
            "sourceVisibleInSubmissionDetail": False,
            "exactByteMatch": False,
            "matchNote": f"提交已完成，但 getcode 回收失败: {exc}",
        }


def make_record(problem: dict[str, Any], path: Path, language: str, row: dict[str, Any], client: YoJClient) -> dict[str, Any]:
    candidate_bytes = path.read_bytes()
    submission_no = int(row["submissionNo"])
    round_trip = recover_source(client, submission_no, candidate_bytes) if row["status"] == "Accepted" else {
        "status": "NOT_APPLICABLE",
        "sourceVisibleInSubmissionDetail": False,
        "exactByteMatch": False,
        "matchNote": "提交结果不是 Accepted，暂不回收源码。",
    }
    return {
        "problemNo": int(problem["problemNo"]),
        "title": str(problem.get("title") or path.parent.name.split("_", 1)[-1]),
        "language": language,
        "submissionNo": submission_no,
        "status": row["status"],
        "score": row["score"],
        "timeMs": row["timeMs"],
        "memoryKiB": row["memoryKiB"],
        "submittedBytes": row["submittedBytes"],
        "submissionDetailUrl": f"{BASE}/index.php/index/submissions/detail/subno/{submission_no}.html",
        "candidatePath": path.relative_to(ROOT).as_posix(),
        "candidateSha256": hashlib.sha256(candidate_bytes).hexdigest(),
        "roundTrip": round_trip,
        "verifiedAt": utc_now(),
    }


def record_skip(skips: dict[int, dict[str, Any]], problem: dict[str, Any], path: Path, language: str, reason: str) -> None:
    skips[int(problem["problemNo"])] = {
        "problemNo": int(problem["problemNo"]),
        "title": str(problem.get("title") or path.parent.name.split("_", 1)[-1]),
        "language": language,
        "candidatePath": path.relative_to(ROOT).as_posix(),
        "reason": reason,
        "recordedAt": utc_now(),
    }


def rebuild_public_index() -> None:
    result = subprocess.run(
        [sys.executable, str(ROOT / "tools" / "build_initial.py")],
        cwd=ROOT,
        capture_output=True,
        text=True,
        timeout=240,
        check=False,
    )
    if result.returncode:
        print(f"  README 构建失败，保留在线记录: {result.stderr[-500:]}", flush=True)


def main() -> int:
    problems, records, skips = load_context()
    candidates = read_candidates()
    client = YoJClient()
    client.login()
    print(f"登录成功；候选题目 {len(candidates)} 道，已有提交证据 {len(records)} 道，已跳过 {len(skips)} 道。", flush=True)
    last_submit_at = 0.0
    completed_since_build = 0
    actual_submissions = 0

    for problem_no in sorted(candidates):
        if problem_no in records or problem_no in skips:
            continue
        path, language = candidates[problem_no]
        problem = problems.get(problem_no, {"problemNo": problem_no, "title": path.parent.name.split("_", 1)[-1]})

        if problem_no in FILL_IN_PROBLEMS:
            record_skip(skips, problem, path, language, "FILL_IN_FRAGMENT_TEMPLATE_UNAVAILABLE")
            save_report(records, skips)
            print(f"[{problem_no}] 跳过：固定模板不可得的填空片段。", flush=True)
            continue

        try:
            page = client.request(f"/index.php/index/problem/detail/pno/{problem_no}.html")
            action, pid = parse_form(page, problem_no, language)
        except Exception as exc:  # noqa: BLE001 - persist the exact gate failure
            reason = str(exc)
            record_skip(skips, problem, path, language, reason)
            save_report(records, skips)
            print(f"[{problem_no}] 跳过：{reason}", flush=True)
            continue

        wait_for = SUBMIT_INTERVAL - (time.monotonic() - last_submit_at)
        if wait_for > 0:
            print(f"[{problem_no}] 等待提交间隔 {wait_for:.1f}s", flush=True)
            time.sleep(wait_for)

        try:
            baseline_rows = fetch_rows(client)
            baseline_max = max((int(row["submissionNo"]) for row in baseline_rows), default=0)
            client.request(
                action,
                {"language": language, "code": path.read_text(encoding="utf-8"), "pid": str(pid)},
                f"/index.php/index/problem/detail/pno/{problem_no}.html",
            )
            last_submit_at = time.monotonic()
            row: dict[str, Any] | None = None
            for _ in range(POLL_LIMIT):
                rows = fetch_rows(client)
                fresh = [item for item in rows if item["submissionNo"] > baseline_max and item["problemNo"] == problem_no]
                if fresh:
                    row = fresh[0]
                    if row["status"].lower() not in PENDING_STATUSES:
                        break
                time.sleep(POLL_INTERVAL)
            if row is None:
                raise RuntimeError("SUBMISSION_STATUS_UNKNOWN")
            if row["status"].lower() in PENDING_STATUSES:
                raise RuntimeError(f"SUBMISSION_STATUS_UNKNOWN:{row['submissionNo']}:{row['status']}")
            records[problem_no] = make_record(problem, path, language, row, client)
            skips.pop(problem_no, None)
            save_report(records, skips)
            actual_submissions += 1
            completed_since_build += 1
            print(
                f"[{problem_no}] {problem.get('title', '')} -> {row['status']} "
                f"#{row['submissionNo']} ({row['timeMs']} ms, {row['memoryKiB']} KB)",
                flush=True,
            )
            if completed_since_build >= BUILD_EVERY:
                rebuild_public_index()
                completed_since_build = 0
        except Exception as exc:  # noqa: BLE001 - unknown submission state must stop
            save_report(records, skips)
            print(f"[{problem_no}] 停止：{exc}。已保存此前进度，不自动重发。", flush=True)
            return 3

    if completed_since_build:
        rebuild_public_index()
    print(f"批处理完成：本次新增提交 {actual_submissions} 道；提交证据总数 {len(records)}；跳过 {len(skips)} 道。", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

