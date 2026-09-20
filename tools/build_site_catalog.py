#!/usr/bin/env python3
"""Build the small, public-safe catalog consumed by the GitHub Pages homepage.

The full problem records stay in ``data/problems.json``.  Pages only needs a
minimal index: title, links, verification state, limits, and an intentionally
conservative first-pass knowledge-point classification derived from titles.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter
from pathlib import Path
from urllib.parse import quote


ROOT = Path(__file__).resolve().parents[1]
PROBLEMS_PATH = ROOT / "data" / "problems.json"
QUICK_SUBMIT_PATH = ROOT / "data" / "quick-submit.json"
QUICK_SUBMIT_PAGES_PATH = ROOT / "docs" / "data" / "quick-submit.json"
OUTPUT_PATH = ROOT / "docs" / "data" / "catalog.json"
GITHUB_BLOB_BASE = "https://github.com/Scorpascal/RUC_YOJ/blob/main/"


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def has_solution_notes(folder: str, problem_no: int) -> bool:
    directory = ROOT / "题解" / folder
    if not directory.exists():
        return False
    markdown_files = list(directory.glob("*.md"))
    extra_file = any(path.name != f"{problem_no:04d}_题目.md" for path in markdown_files)
    if extra_file:
        return True
    statement = directory / f"{problem_no:04d}_题目.md"
    if not statement.exists():
        return False
    text = statement.read_text(encoding="utf-8", errors="ignore")
    return bool(re.search(r"^##\s+(思路|解法|算法|复杂度|题解)\s*$", text, re.MULTILINE))


def infer_tags(title: str) -> list[str]:
    """Return a deliberately small title-based classification.

    This is a navigation aid, not a claim that the problem has been manually
    annotated.  The site explains that these tags are provisional.
    """

    text = title.lower()
    groups = [
        ("数据结构", ("树", "tree", "avl", "红黑", "链表", "栈", "队列", "堆", "并查", "trie", "hash", "集合", "容器")),
        ("图论", ("图", "graph", "路径", "path", "网络", "拓扑", "最短", "连通", "匹配", "流", "dijkstra", "floyd", "bfs", "dfs")),
        ("动态规划", ("动态规划", "dp", "背包", "序列", "子序列", "状态转移")),
        ("字符串", ("字符串", "字符", "子串", "kmp", "回文", "isbn", "ascii")),
        ("搜索与排序", ("二分", "排序", "查找", "搜索", "众数", "中位数", "合并", "选择")),
        ("数学", ("数", "矩阵", "方程", "质因", "素数", "分数", "fibonacci", "进制", "日期", "几何", "三角", "统计", "cos")),
    ]
    tags: list[str] = []
    for label, keywords in groups:
        if any(keyword in text for keyword in keywords):
            tags.append(label)
    return tags[:2] or ["基础语法"]


def github_blob(path: str | None) -> str | None:
    if not path:
        return None
    return GITHUB_BLOB_BASE + quote(path.replace("\\", "/"), safe="/")


def build_payload() -> dict:
    problems = load_json(PROBLEMS_PATH)
    quick = load_json(QUICK_SUBMIT_PATH)
    quick_by_no = {int(item["problemNo"]): item for item in quick.get("entries", [])}

    entries: list[dict] = []
    tag_counts: Counter[str] = Counter()
    for record in problems.get("records", []):
        problem_no = int(record["problemNo"])
        quick_entry = quick_by_no.get(problem_no, {})
        title = str(record.get("title") or quick_entry.get("title") or f"题目 {problem_no}")
        folder = str(record.get("folder") or f"{problem_no:04d}_{title}")
        public = record.get("public") or {}
        limits = record.get("limits") or {}
        statement_path = public.get("statement") or f"题解/{folder}/{problem_no:04d}_题目.md"
        tags = infer_tags(title)
        for tag in tags:
            tag_counts[tag] += 1
        verified = public.get("status") == "PUBLIC_READY" and quick_entry.get("onlineStatus") == "Accepted"
        has_notes = has_solution_notes(folder, problem_no)
        entries.append(
            {
                "problemNo": problem_no,
                "title": title,
                "folder": folder,
                "tags": tags,
                "language": quick_entry.get("language") or record.get("language") or "—",
                "timeLimit": limits.get("time") or "—",
                "memoryLimit": limits.get("memory") or "—",
                "verified": verified,
                "solutionStatus": "solution" if has_notes else "statement",
                "problemUrl": quick_entry.get("problemUrl") or record.get("problemUrl"),
                "statementUrl": github_blob(statement_path),
                "codeUrl": quick_entry.get("codeUrl"),
                "quickSubmitUrl": f"./yoj-quick-submit.html?pno={problem_no}" if verified else None,
                "submissionNo": quick_entry.get("onlineSubmissionNo"),
            }
        )

    entries.sort(key=lambda item: item["problemNo"])
    return {
        "schemaVersion": 1,
        "generatedAt": quick.get("generatedAt"),
        "source": {
            "problemRecords": len(entries),
            "verifiedSubmissions": sum(1 for item in entries if item["verified"]),
            "tagging": "title-heuristic-v1",
            "note": "知识点标签为根据题名生成的初步导航标签，需逐题人工校准。",
        },
        "tagCounts": dict(sorted(tag_counts.items(), key=lambda pair: (-pair[1], pair[0]))),
        "entries": entries,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="生成或校验 GitHub Pages 题库目录")
    parser.add_argument(
        "--check",
        action="store_true",
        help="只校验已提交的 docs/data/catalog.json 是否与当前数据一致，不写文件",
    )
    args = parser.parse_args()

    payload = build_payload()
    rendered = json.dumps(payload, ensure_ascii=False, indent=2) + "\n"
    if args.check:
        expected_quick = QUICK_SUBMIT_PATH.read_text(encoding="utf-8") if QUICK_SUBMIT_PATH.is_file() else None
        actual_quick = (
            QUICK_SUBMIT_PAGES_PATH.read_text(encoding="utf-8")
            if QUICK_SUBMIT_PAGES_PATH.is_file()
            else None
        )
        if expected_quick is None or actual_quick != expected_quick:
            print(
                "quick-submit drift: run `python3 tools/build_initial.py` and commit "
                "data/quick-submit.json and docs/data/quick-submit.json together",
                file=sys.stderr,
            )
            return 1
        if not OUTPUT_PATH.is_file():
            print(f"catalog missing: {OUTPUT_PATH}", file=sys.stderr)
            return 1
        current = OUTPUT_PATH.read_text(encoding="utf-8")
        if current != rendered:
            try:
                current_payload = json.loads(current)
                current_source = current_payload.get("source") or {}
                current_count = len(current_payload.get("entries") or [])
                current_verified = current_source.get("verifiedSubmissions")
            except (TypeError, ValueError, json.JSONDecodeError):
                current_count = "invalid"
                current_verified = "invalid"
            expected_source = payload["source"]
            print(
                "catalog drift: run `python3 tools/build_site_catalog.py` and commit "
                f"the result; expected entries={len(payload['entries'])}, "
                f"verified={expected_source['verifiedSubmissions']}, "
                f"actual entries={current_count}, verified={current_verified}",
                file=sys.stderr,
            )
            return 1
        print(
            f"catalog ok: {len(payload['entries'])} entries, "
            f"{payload['source']['verifiedSubmissions']} verified -> {OUTPUT_PATH}"
        )
        return 0

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(rendered, encoding="utf-8")
    print(f"built {len(payload['entries'])} entries -> {OUTPUT_PATH}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
