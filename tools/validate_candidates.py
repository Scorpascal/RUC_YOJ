#!/usr/bin/env python3
"""Run local syntax gates for captured solution candidates.

This is deliberately a syntax-only gate.  It does not invent tests, submit to
YOJ, or change the immutable ``代码库/`` archive.  Results are written to the
ignored ``staging/`` directory unless ``--check`` is supplied.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import platform
import subprocess
import sys
from collections import Counter
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DATA_PATH = ROOT / "data" / "problems.json"
RAW_ROOT = ROOT / "代码库"
CANDIDATE_ROOT = ROOT / "staging" / "cpp17-compatible"
CLEANED_ROOT = ROOT / "staging" / "cleaned-code"
PORTABLE_ROOT = ROOT / "staging" / "cpp17-portable"
REPAIRED_ROOT = ROOT / "staging" / "repaired-code"
REPORT_PATH = ROOT / "staging" / "local-validation-report.json"

# These two submissions are function-body fragments.  Their fixed templates
# are hidden by YOJ, so compiling them as standalone programs would require
# inventing a wrapper and would not be evidence for a real submission.
KNOWN_FILL_IN_FRAGMENTS = {"285", "286"}


def classify_diagnostic(stderr: str, returncode: int) -> str:
    if returncode == 0:
        return "PASS"
    if "pb_ds" in stderr or "ext/pb_ds" in stderr:
        return "GNU_PB_DS_MISSING"
    if "bits/stdc++.h" in stderr:
        return "BITS_STILL_PRESENT"
    if "file not found" in stderr:
        return "MISSING_HEADER"
    if "timed out" in stderr.lower():
        return "TIMEOUT"
    if "error:" in stderr:
        return "CODE_OR_STANDARD_ERROR"
    return "OTHER_COMPILER_FAILURE"


def run_compiler(command: list[str], timeout: int = 30) -> tuple[int, str]:
    try:
        result = subprocess.run(
            command,
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired as exc:
        detail = str(exc.stderr or "")
        return 124, f"compiler timeout after {timeout}s\n{detail}"
    except OSError as exc:
        return 127, f"compiler unavailable: {exc}"
    return result.returncode, result.stderr


def source_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def candidate_for(raw_path: Path) -> Path:
    relative = raw_path.relative_to(RAW_ROOT)
    for root in (REPAIRED_ROOT, PORTABLE_ROOT, CLEANED_ROOT, CANDIDATE_ROOT):
        candidate = root / relative
        if candidate.is_file():
            return candidate
    return raw_path


def validate_record(record: dict[str, Any]) -> dict[str, Any]:
    archive = record.get("archive") or {}
    if not archive.get("completeCode"):
        return {
            "problemNo": str(record["problemNo"]),
            "title": str(record["title"]),
            "language": str(record.get("language") or ""),
            "source": None,
            "candidateSha256": "",
            "scope": "topic_capture_without_local_accepted_source",
            "category": "TOPIC_ONLY",
            "returncode": None,
            "command": None,
            "diagnostic": "题面已归档，但尚无本人 Accepted 源码；跳过代码语法门禁。",
        }
    raw_path = ROOT / str(archive["completeCode"])
    language = str(record.get("language") or "")
    source = candidate_for(raw_path)
    result: dict[str, Any] = {
        "problemNo": str(record["problemNo"]),
        "title": str(record["title"]),
        "language": language,
        "source": source.relative_to(ROOT).as_posix(),
        "candidateSha256": source_sha256(source),
        "scope": "complete_source_syntax_only",
    }
    if result["problemNo"] in KNOWN_FILL_IN_FRAGMENTS:
        result.update(
            {
                "scope": "fill_in_fragment_not_standalone",
                "category": "FILL_IN_FRAGMENT",
                "returncode": None,
                "command": None,
                "diagnostic": "题面固定模板下载被隐藏；当前文件只有函数体片段，未臆造 main 或模板外壳。",
            }
        )
        return result
    if language.startswith("cpp"):
        command = ["clang++", "-std=c++17", "-fsyntax-only", str(source)]
        returncode, stderr = run_compiler(command)
        result.update(
            {
                "command": command,
                "returncode": returncode,
                "category": classify_diagnostic(stderr, returncode),
                "diagnostic": "\n".join(stderr.splitlines()[:12]),
            }
        )
        return result
    if language.startswith("c-") or language == "c":
        command = ["clang", "-std=c17", "-fsyntax-only", str(source)]
        returncode, stderr = run_compiler(command)
        result.update(
            {
                "command": command,
                "returncode": returncode,
                "category": classify_diagnostic(stderr, returncode),
                "diagnostic": "\n".join(stderr.splitlines()[:12]),
            }
        )
        return result
    if language.startswith("python"):
        try:
            compile(source.read_text(encoding="utf-8"), str(source), "exec")
        except (OSError, SyntaxError) as exc:
            result.update(
                {
                    "category": "PYTHON_SYNTAX_ERROR",
                    "returncode": 1,
                    "diagnostic": str(exc),
                }
            )
        else:
            result.update(
                {
                    "category": "PASS",
                    "returncode": 0,
                    "diagnostic": "",
                }
            )
        return result
    result.update(
        {
            "category": "UNSUPPORTED_LANGUAGE",
            "returncode": None,
            "diagnostic": "未配置该语言的本地语法检查器",
        }
    )
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="对归档完整代码运行本地语法门禁")
    parser.add_argument("--check", action="store_true", help="只运行并打印检查，不写报告")
    parser.add_argument("--problem", dest="problem_nos", action="append", type=int, help="只处理指定题号")
    args = parser.parse_args()

    records = json.loads(DATA_PATH.read_text(encoding="utf-8"))["records"]
    selected = set(args.problem_nos or [])
    results = [
        validate_record(record)
        for record in records
        if not selected or int(record["problemNo"]) in selected
    ]
    counts = Counter(item["category"] for item in results)
    report = {
        "schemaVersion": 1,
        "purpose": "local syntax gate; no sample tests or online submissions",
        "python": sys.version,
        "platform": platform.platform(),
        "records": len(results),
        "counts": dict(sorted(counts.items())),
        "results": results,
        "failures": [item for item in results if item["category"] not in {"PASS", "TOPIC_ONLY", "FILL_IN_FRAGMENT"}],
        "notes": [
            "完整代码通过语法检查不代表样例、边界、分块回环或 YOJ 在线 AC。",
            "填空片段若没有固定模板，按片段失败或未独立编译处理，不自动补 main。",
            "GNU ext/pb_ds 缺失时保留为工具链兼容性问题，不把失败误记为算法错误。",
        ],
    }
    if not args.check:
        REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
        REPORT_PATH.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"records": len(results), "counts": dict(sorted(counts.items()))}, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
