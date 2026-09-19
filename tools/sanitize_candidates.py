#!/usr/bin/env python3
"""Create conservative de-personalized code candidates.

Only comments containing explicitly configured personal/path markers are
removed or neutralized.  Strings, identifiers, templates, and output text are
never rewritten by this tool.  The raw archive stays byte-for-byte unchanged.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
RAW_ROOT = ROOT / "代码库"
DATA_PATH = ROOT / "data" / "problems.json"
NORMALIZED_ROOT = ROOT / "staging" / "cpp17-compatible"
PORTABLE_ROOT = ROOT / "staging" / "cpp17-portable"
REPAIRED_ROOT = ROOT / "staging" / "repaired-code"
OUTPUT_ROOT = ROOT / "staging" / "cleaned-code"
REPORT_PATH = ROOT / "staging" / "sanitization-report.json"

# 个人标识词条不写入源码（本仓库公开）。改为运行时从本地词表加载，
# 词表文件已被 .gitignore 排除，不会进入 Git 历史或 GitHub。
PERSONAL_TERMS_FILE = ROOT / "method" / "personal_terms.txt"

# 通用（不含个人信息）的本地路径标记，可安全留在源码中。
GENERIC_MARKERS = (
    re.compile(r"/Users/[^\s*]+"),
    re.compile(r"/var/folders/[^\s*]+"),
    re.compile(r"[A-Za-z]:\\Users\\[^\s*]+", re.IGNORECASE),
)


def load_personal_markers() -> tuple[re.Pattern[str], ...]:
    """从本地词表加载个人标识正则，每行一条，`#` 开头为注释。

    词表缺失时只使用通用路径规则，并给出提示，避免静默漏脱敏。
    """
    patterns: list[re.Pattern[str]] = []
    if PERSONAL_TERMS_FILE.exists():
        for line in PERSONAL_TERMS_FILE.read_text(encoding="utf-8").splitlines():
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            patterns.append(re.compile(line))
    else:
        print(
            f"[warn] 未找到个人词表 {PERSONAL_TERMS_FILE}，本次只应用通用路径规则。",
            file=sys.stderr,
        )
    return tuple(patterns)


PERSONAL_MARKERS = load_personal_markers() + GENERIC_MARKERS


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def has_personal_marker(value: str) -> bool:
    return any(pattern.search(value) for pattern in PERSONAL_MARKERS)


def sanitize_comment(comment: str) -> tuple[str, bool]:
    if not has_personal_marker(comment):
        return comment, False
    # A comment has no program semantics.  Removing the complete comment is
    # safer than replacing a personal token inside a string-like code example.
    if comment.startswith("//") or comment.startswith("#"):
        return "", True
    newline_count = comment.count("\n")
    return "\n" * newline_count, True


def sanitize_comments(text: str, python_comment: bool) -> tuple[str, int]:
    """Remove configured markers from C/C++/Python comments only."""

    output: list[str] = []
    i = 0
    changed = 0
    state = "normal"
    quote = ""
    while i < len(text):
        char = text[i]
        next_char = text[i + 1] if i + 1 < len(text) else ""
        if state == "normal":
            if char in {'"', "'", "`"}:
                state = "string"
                quote = char
                output.append(char)
                i += 1
                continue
            if char == "/" and next_char == "/":
                end = text.find("\n", i)
                if end == -1:
                    end = len(text)
                comment, did_change = sanitize_comment(text[i:end])
                output.append(comment)
                changed += int(did_change)
                i = end
                continue
            if char == "/" and next_char == "*":
                end = text.find("*/", i + 2)
                end = len(text) if end == -1 else end + 2
                comment, did_change = sanitize_comment(text[i:end])
                output.append(comment)
                changed += int(did_change)
                i = end
                continue
            if python_comment and char == "#":
                end = text.find("\n", i)
                if end == -1:
                    end = len(text)
                comment, did_change = sanitize_comment(text[i:end])
                output.append(comment)
                changed += int(did_change)
                i = end
                continue
            output.append(char)
            i += 1
            continue
        output.append(char)
        if char == "\\" and i + 1 < len(text):
            output.append(text[i + 1])
            i += 2
            continue
        if char == quote:
            state = "normal"
            quote = ""
        i += 1
    return "".join(output), changed


def source_path_for(raw_path: Path) -> Path:
    relative = raw_path.relative_to(RAW_ROOT)
    for root in (REPAIRED_ROOT, PORTABLE_ROOT, NORMALIZED_ROOT):
        candidate = root / relative
        if candidate.is_file():
            return candidate
    return raw_path


def main() -> int:
    parser = argparse.ArgumentParser(description="生成只清理个性化注释的代码候选")
    parser.add_argument("--check", action="store_true", help="只统计命中，不写候选")
    args = parser.parse_args()

    records = json.loads(DATA_PATH.read_text(encoding="utf-8"))["records"]
    seen: set[str] = set()
    files: list[dict[str, Any]] = []
    for record in records:
        archive = record["archive"]
        for key in ("completeCode", "directlySubmittableCode"):
            raw_path = ROOT / str(archive[key])
            raw_key = raw_path.relative_to(ROOT).as_posix()
            if raw_key in seen:
                continue
            seen.add(raw_key)
            source = source_path_for(raw_path)
            original = source.read_text(encoding="utf-8")
            cleaned, changed_comments = sanitize_comments(
                original,
                python_comment=source.suffix.lower() == ".py",
            )
            output_path = OUTPUT_ROOT / raw_path.relative_to(RAW_ROOT)
            if not args.check:
                output_path.parent.mkdir(parents=True, exist_ok=True)
                output_path.write_text(cleaned, encoding="utf-8", newline="\n")
            files.append(
                {
                    "problemNo": str(record["problemNo"]),
                    "language": str(record.get("language") or ""),
                    "source": source.relative_to(ROOT).as_posix(),
                    "candidate": output_path.relative_to(ROOT).as_posix(),
                    "sourceSha256": sha256(original.encode("utf-8")),
                    "candidateSha256": sha256(cleaned.encode("utf-8")),
                    "changedComments": changed_comments,
                    "changed": cleaned != original,
                    "remainingConfiguredMarkers": [
                        # 只记录命中序号，不写出词条本身，避免报告反向泄露个人信息。
                        f"marker#{idx}"
                        for idx, pattern in enumerate(PERSONAL_MARKERS)
                        if pattern.search(cleaned)
                    ],
                }
            )

    report = {
        "schemaVersion": 1,
        "purpose": "conservative comment-only de-personalization candidate",
        "rawArchiveChangedByThisTool": False,
        "files": len(files),
        "changedFiles": sum(item["changed"] for item in files),
        "changedComments": sum(item["changedComments"] for item in files),
        "remainingConfiguredMarkerFiles": sum(bool(item["remainingConfiguredMarkers"]) for item in files),
        "filesWithRemainingConfiguredMarkers": [item for item in files if item["remainingConfiguredMarkers"]],
        "notes": [
            "只处理注释；字符串、标识符、题目要求的输出和固定模板保持不变。",
            "变量名和输出字符串需要单独的语义复核，不能用全局替换自动改写。",
            "候选未经过样例/边界测试、分块回环或在线 AC，不具备 PUBLIC_READY 资格。",
        ],
        "fileRecords": files,
    }
    if not args.check:
        REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
        REPORT_PATH.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(
        json.dumps(
            {
                "files": len(files),
                "changedFiles": report["changedFiles"],
                "changedComments": report["changedComments"],
                "remainingConfiguredMarkerFiles": report["remainingConfiguredMarkerFiles"],
            },
            ensure_ascii=False,
            indent=2,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
