#!/usr/bin/env python3
"""Compile and link complete code candidates without submitting them.

This is separate from the syntax gate: it catches missing symbols and linker
errors while keeping all generated binaries in a temporary directory.  The
raw archive and public readiness fields are not changed.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import platform
import subprocess
import sys
import tempfile
from collections import Counter
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DATA_PATH = ROOT / "data" / "problems.json"
RAW_ROOT = ROOT / "代码库"
REPAIRED_ROOT = ROOT / "staging" / "repaired-code"
PORTABLE_ROOT = ROOT / "staging" / "cpp17-portable"
CLEANED_ROOT = ROOT / "staging" / "cleaned-code"
NORMALIZED_ROOT = ROOT / "staging" / "cpp17-compatible"
REPORT_PATH = ROOT / "staging" / "local-compile-report.json"
KNOWN_FILL_IN_FRAGMENTS = {"285", "286"}


def candidate_path(raw_path: Path) -> Path:
    relative = raw_path.relative_to(RAW_ROOT)
    for root in (REPAIRED_ROOT, PORTABLE_ROOT, CLEANED_ROOT, NORMALIZED_ROOT):
        candidate = root / relative
        if candidate.is_file():
            return candidate
    return raw_path


def compile_command(record: dict[str, Any], source: Path, executable: Path) -> list[str] | None:
    language = str(record.get("language") or "")
    if language.startswith("cpp"):
        return ["clang++", "-std=c++17", "-Wall", "-Wextra", "-pedantic", str(source), "-o", str(executable)]
    if language.startswith("c-") or language == "c":
        return ["clang", "-std=c17", "-Wall", "-Wextra", "-pedantic", str(source), "-o", str(executable)]
    return None


def classify(returncode: int, stderr: str) -> str:
    if returncode == 0:
        return "PASS"
    lowered = stderr.lower()
    if "undefined symbol" in lowered or "undefined reference" in lowered:
        return "LINK_ERROR"
    if "pb_ds" in stderr or "ext/pb_ds" in stderr:
        return "GNU_PB_DS_MISSING"
    if "bits/stdc++.h" in stderr:
        return "BITS_STILL_PRESENT"
    if "file not found" in lowered:
        return "MISSING_HEADER"
    return "COMPILE_ERROR"


def run_command(command: list[str], timeout: int = 30) -> tuple[int, str]:
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
        return 124, f"compiler timeout after {timeout}s\n{exc.stderr or ''}"
    except OSError as exc:
        return 127, f"compiler unavailable: {exc}"
    return result.returncode, result.stderr


def validate_python(source: Path) -> tuple[str, str]:
    try:
        compile(source.read_text(encoding="utf-8"), str(source), "exec")
    except (OSError, SyntaxError) as exc:
        return "PYTHON_SYNTAX_ERROR", str(exc)
    return "PASS", ""


def main() -> int:
    parser = argparse.ArgumentParser(description="对完整代码候选执行本地编译/链接门禁")
    parser.add_argument("--check", action="store_true", help="运行检查但不写报告")
    args = parser.parse_args()

    records = json.loads(DATA_PATH.read_text(encoding="utf-8"))["records"]
    results: list[dict[str, Any]] = []
    with tempfile.TemporaryDirectory(prefix="yoj-compile-") as temporary:
        binary_root = Path(temporary)
        for record in records:
            problem_no = str(record["problemNo"])
            raw_source = ROOT / str(record["archive"]["completeCode"])
            source = candidate_path(raw_source)
            language = str(record.get("language") or "")
            item: dict[str, Any] = {
                "problemNo": problem_no,
                "title": str(record["title"]),
                "language": language,
                "source": source.relative_to(ROOT).as_posix(),
                "scope": "complete_source_compile_link",
            }
            if problem_no in KNOWN_FILL_IN_FRAGMENTS:
                item.update(
                    {
                        "category": "FILL_IN_FRAGMENT",
                        "returncode": None,
                        "command": None,
                        "diagnostic": "固定模板不可得，未臆造 main 或完整提交外壳。",
                    }
                )
                results.append(item)
                continue
            if language.startswith("python"):
                category, diagnostic = validate_python(source)
                item.update(
                    {
                        "category": category,
                        "returncode": 0 if category == "PASS" else 1,
                        "command": [sys.executable, "compile()"],
                        "diagnostic": diagnostic,
                    }
                )
                results.append(item)
                continue
            executable = binary_root / ("program-" + hashlib.sha256(str(source).encode()).hexdigest()[:16])
            command = compile_command(record, source, executable)
            if command is None:
                item.update(
                    {
                        "category": "UNSUPPORTED_LANGUAGE",
                        "returncode": None,
                        "command": None,
                        "diagnostic": "未配置该语言的编译器",
                    }
                )
                results.append(item)
                continue
            returncode, stderr = run_command(command)
            item.update(
                {
                    "category": classify(returncode, stderr),
                    "returncode": returncode,
                    "command": command,
                    "diagnostic": "\n".join(stderr.splitlines()[:16]),
                }
            )
            results.append(item)

    counts = Counter(item["category"] for item in results)
    report = {
        "schemaVersion": 1,
        "purpose": "local compile/link gate; no sample tests, online submissions, or publication",
        "python": sys.version,
        "platform": platform.platform(),
        "records": len(results),
        "counts": dict(sorted(counts.items())),
        "failures": [item for item in results if item["category"] != "PASS"],
        "notes": [
            "C/C++ 使用 Clang C++17/C17 实际编译并链接；Python 只做 compile() 语法门禁。",
            "填空片段、样例、边界、分块回环和 YOJ 在线 Accepted 仍需分别处理。",
            "临时可执行文件在本次运行结束时清理，原始代码库不被覆盖。",
        ],
    }
    if not args.check:
        REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
        REPORT_PATH.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"records": len(results), "counts": dict(sorted(counts.items()))}, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
