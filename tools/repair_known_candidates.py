#!/usr/bin/env python3
"""Create narrowly scoped repaired candidates for confirmed portability bugs.

The raw YOJ archive is never overwritten.  Each repair is explicit, checked
against the captured source shape, and written only to ignored ``staging/``.
The resulting program is still a candidate until sample, boundary and online
verification gates pass.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
RAW_ROOT = ROOT / "代码库"
DATA_PATH = ROOT / "data" / "problems.json"
OUTPUT_ROOT = ROOT / "staging" / "repaired-code"
REPORT_PATH = ROOT / "staging" / "known-repair-report.json"


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def p1546_solution() -> str:
    """Use typed C++ stream extraction instead of mismatched scanf pointers."""

    return """#include <iostream>

int main() {
    long long base;
    long long digit1;
    long long digit2;
    long long digit3;
    long long digit4;
    if (!(std::cin >> base >> digit1 >> digit2 >> digit3 >> digit4)) {
        return 0;
    }

    std::cout << digit4 + digit3 * base + digit2 * base * base
              + digit1 * base * base * base << '\\n';
    return 0;
}
"""


REPAIRS: dict[str, dict[str, Any]] = {
    "1546": {
        "reason": "原代码把 long long* 传给 scanf 的 %d，属于未定义行为；改用 C++17 类型安全输入。",
        "must_contain": 'scanf("%d%d%d%d%d",&k,&a,&b,&c,&d);',
        "solution": p1546_solution,
    },
}


def main() -> int:
    parser = argparse.ArgumentParser(description="生成已确认问题的 C++17 候选修正版")
    parser.add_argument("--check", action="store_true", help="只检查修复前提，不写候选报告")
    parser.add_argument("--problem", dest="problem_nos", action="append", type=int, help="只处理指定题号")
    args = parser.parse_args()

    records = json.loads(DATA_PATH.read_text(encoding="utf-8"))["records"]
    selected = set(args.problem_nos or [])
    file_records: list[dict[str, Any]] = []
    repaired_problems: set[str] = set()

    for record in records:
        problem_no = str(record["problemNo"])
        if selected and int(problem_no) not in selected:
            continue
        repair = REPAIRS.get(problem_no)
        if repair is None:
            continue
        if not (record.get("archive") or {}).get("completeCode"):
            continue
        repaired_problems.add(problem_no)
        candidate_text = str(repair["solution"]())
        for key in ("completeCode", "directlySubmittableCode"):
            raw_path = ROOT / str(record["archive"][key])
            raw_bytes = raw_path.read_bytes()
            raw_text = raw_bytes.decode("utf-8")
            required = str(repair["must_contain"])
            already_fixed = raw_text == candidate_text
            if required not in raw_text and not already_fixed:
                raise SystemExit(
                    f"repair precondition failed for {problem_no} {key}: "
                    f"expected source fragment not found in {raw_path}"
                )

            output_path = OUTPUT_ROOT / raw_path.relative_to(RAW_ROOT)
            candidate_bytes = candidate_text.encode("utf-8")
            if not args.check:
                output_path.parent.mkdir(parents=True, exist_ok=True)
                output_path.write_bytes(candidate_bytes)
            file_records.append(
                {
                    "problemNo": problem_no,
                    "source": raw_path.relative_to(ROOT).as_posix(),
                    "candidate": output_path.relative_to(ROOT).as_posix(),
                    "reason": repair["reason"],
                    "sourceSha256": sha256(raw_bytes),
                    "candidateSha256": sha256(candidate_bytes),
                    "sourceBytes": len(raw_bytes),
                    "candidateBytes": len(candidate_bytes),
                    "precondition": "ALREADY_REPAIRED_IN_ARCHIVE" if already_fixed else "ORIGINAL_FRAGMENT_FOUND",
                }
            )

    report = {
        "schemaVersion": 1,
        "purpose": "explicit candidate-only repairs for confirmed portability/undefined-behavior issues",
        "rawArchiveChangedByThisTool": False,
        "problems": sorted(repaired_problems, key=int),
        "files": len(file_records),
        "fileRecords": file_records,
        "notes": [
            "当前仅修复 1546 题 scanf 类型不匹配；未改写代码库中的原始文件。",
            "候选仍需样例、边界、在线 AC 及回收源码比对，不能直接标记 PUBLIC_READY。",
        ],
    }
    if not args.check:
        REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
        REPORT_PATH.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(
        json.dumps(
            {"problems": report["problems"], "files": report["files"], "rawArchiveChangedByThisTool": False},
            ensure_ascii=False,
            indent=2,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
