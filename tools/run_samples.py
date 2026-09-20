#!/usr/bin/env python3
"""Run conservatively extracted statement samples against local candidates.

This is a supplementary test only.  It never sends a YOJ request and never
turns a sample pass into an Accepted or publication status.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import re
import shutil
import subprocess
import sys
from collections import Counter
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DATA_PATH = ROOT / "data" / "problems.json"
STATEMENT_ROOT = ROOT / "题解"
RAW_ROOT = ROOT / "代码库"
PORTABLE_ROOT = ROOT / "staging" / "cpp17-portable"
REPAIRED_ROOT = ROOT / "staging" / "repaired-code"
CLEANED_ROOT = ROOT / "staging" / "cleaned-code"
NORMALIZED_ROOT = ROOT / "staging" / "cpp17-compatible"
REPORT_PATH = ROOT / "staging" / "sample-test-report.json"
WORK_ROOT = ROOT / "staging" / "sample-tests"
KNOWN_FILL_IN_FRAGMENTS = {"285", "286"}
MAX_OUTPUT_CHARS = 1_000_000

# These are statement-side defects visible in the captured samples.  They
# must not be “fixed” by changing an otherwise independently captured
# solution: the correct action is to keep the sample out of the pass/fail
# regression count and retain the reason in the report.
SAMPLE_QUALITY_ISSUES = {
    "87": "输入样例声明 n=5，但输出样例包含 6 行，和题面输出格式矛盾",
    "284": "输入样例声明 N=5 却只给出 4 条记录，输出还出现输入中不存在的 alice 331",
}
# 1645 的规则与样例含义没有歧义：原始 HTML 只丢失了样例末尾那个
# 不可见的 ASCII 空格。离线回归在输入副本中补回该空格，并在报告中
# 留下说明；原始 HTML 和公开题面仍然原样保留。
SAMPLE_CORRECTIONS = {
    "1645": "原始 HTML 样例末尾缺少规则要求的 ASCII 空格，离线回归已在输入副本末尾补回",
}
FLOAT_TOKEN_RE = re.compile(r"[.eE]")


def statement_path(record: dict[str, Any]) -> Path:
    return ROOT / str(record["public"]["statement"])


def is_heading(line: str, keyword: str) -> bool:
    stripped = line.strip().strip("#* ").strip("【】").strip()
    return stripped in {keyword, f"{keyword}：", f"{keyword}:"}


def sample_marker(line: str, keyword: str) -> bool:
    stripped = line.strip().strip("#* ").strip("【】").strip().rstrip("：:")
    base = keyword.replace("样例", "")
    aliases = (keyword, f"样例{base}")
    for alias in aliases:
        if stripped == alias:
            return True
        suffix = stripped[len(alias) :] if stripped.startswith(alias) else ""
        if suffix and all(char.isdigit() or char in "一二三四五六七八九十" for char in suffix):
            return True
    return False


def extract_section(lines: list[str], start_keyword: str, stop_keywords: tuple[str, ...]) -> str:
    start = None
    for index, line in enumerate(lines):
        if sample_marker(line, start_keyword):
            start = index + 1
            break
    if start is None:
        return ""
    collected: list[str] = []
    in_fence = False
    for line in lines[start:]:
        if line.strip().startswith("```"):
            if in_fence:
                in_fence = False
            elif any(item.strip() for item in collected):
                break
            else:
                in_fence = True
            continue
        if not in_fence and sample_marker(line, start_keyword):
            if any(item.strip() for item in collected):
                break
            continue
        if not in_fence and any(sample_marker(line, keyword) for keyword in stop_keywords):
            break
        if not in_fence:
            if line.strip() == "****":
                break
            normalized = line.strip().strip("#* ").strip("【】").strip()
            if (
                "提示" in normalized
                or normalized.startswith("注意")
                or normalized.startswith("注释")
                or normalized.startswith("解读")
                or normalized.startswith("请")
                or normalized.startswith("参考")
                or normalized.startswith("以下")
                or normalized.startswith("对于")
                or normalized.startswith("说明")
                or normalized.startswith("来源")
                or normalized.startswith("功能")
                or normalized.startswith("结构参见")
                or normalized.startswith("输入范围")
                or normalized.startswith("解释")
                or normalized.startswith("输出两个")
                or normalized.startswith("100%的数据")
                or "数据范围" in normalized
                or "数据规模" in normalized
                or "数据描述" in normalized
                or "特别说明" in normalized
                or "输入数据的含义" in normalized
                or "参考资料" in normalized
            ):
                break
            if "样例解释" in normalized or "样例说明" in normalized or "归档状态" in normalized:
                break
            if "原题字符待核对" in normalized or "公式待核对" in normalized:
                break
            if normalized.startswith("![") or normalized.startswith("["):
                break
            has_content = any(item.strip() for item in collected)
            if normalized.startswith("样例") and not has_content:
                continue
            if normalized.startswith("样例") and has_content:
                break
        if not in_fence and (
            line.strip() in {"---", "## 归档状态", "## 题面下载资源"}
            or (line.strip() and set(line.strip()) == {"-"})
        ):
            break
        collected.append(line)
    return "\n".join(collected).strip()


def extract_sample_pair(path: Path) -> tuple[str, str, str]:
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        return "", "", f"statement_read_error: {exc}"
    sample_input = extract_section(lines, "输入样例", ("输出样例",))
    sample_output = extract_section(lines, "输出样例", ("输入样例",))
    if not sample_input or not sample_output:
        return "", "", "sample_pair_not_found"
    # HTML often turns ordinary spaces into NBSP/thin-space characters.
    # C/C++ scanf and many older judges do not treat those bytes as separators.
    def normalize_sample_space(value: str) -> str:
        return value.replace("\u00a0", " ").replace("\u202f", " ")

    return normalize_sample_space(sample_input) + "\n", normalize_sample_space(sample_output) + "\n", ""


def compact_html_formatting_blank_lines(value: str) -> tuple[str, bool]:
    """Remove blank lines introduced by one HTML paragraph per sample row."""

    lines = value.splitlines()
    if not lines or not any(not line.strip() for line in lines):
        return value, False
    compacted = "\n".join(line for line in lines if line.strip())
    return compacted + "\n", True


def outputs_match(actual: str, expected: str) -> tuple[bool, str]:
    actual_tokens = normalize_output(actual)
    expected_tokens = normalize_output(expected)
    if actual_tokens == expected_tokens:
        return True, "EXACT"
    if len(actual_tokens) != len(expected_tokens):
        return False, ""
    # Approximate-value problems commonly print a valid value that differs
    # from the page's rounded sample.  Apply a small numeric tolerance only
    # when at least one token visibly contains a decimal/exponent; integers
    # and ordinary text remain exact comparisons.
    if not any(FLOAT_TOKEN_RE.search(token) for token in expected_tokens + actual_tokens):
        return False, ""
    for actual_token, expected_token in zip(actual_tokens, expected_tokens):
        try:
            actual_value = float(actual_token)
            expected_value = float(expected_token)
        except ValueError:
            return False, ""
        tolerance = max(1e-3, 1e-6 * max(abs(actual_value), abs(expected_value), 1.0))
        if abs(actual_value - expected_value) > tolerance:
            return False, ""
    return True, "NUMERIC_TOLERANCE"


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
        return ["clang++", "-std=c++17", "-O2", "-pipe", str(source), "-o", str(executable)]
    if language == "c" or language.startswith("c-"):
        return ["clang", "-std=c17", "-O2", "-pipe", str(source), "-o", str(executable)]
    return None


def run_process(command: list[str], input_data: str, timeout: int) -> tuple[str, str, int | None, str]:
    try:
        process_env = os.environ.copy()
        process_env["PATH"] = os.environ.get("PATH", "")
        result = subprocess.run(
            command,
            cwd=WORK_ROOT,
            input=input_data,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout,
            env=process_env,
        )
    except subprocess.TimeoutExpired as exc:
        return str(exc.stdout or "")[:MAX_OUTPUT_CHARS], str(exc.stderr or "")[:MAX_OUTPUT_CHARS], None, "TIMEOUT"
    except OSError as exc:
        return "", str(exc), None, "PROCESS_ERROR"
    return result.stdout[:MAX_OUTPUT_CHARS], result.stderr[:MAX_OUTPUT_CHARS], result.returncode, ""


def normalize_output(value: str) -> list[str]:
    return value.replace("\r\n", "\n").replace("\r", "\n").split()


def test_record(record: dict[str, Any], run: bool) -> dict[str, Any]:
    problem_no = str(record["problemNo"])
    if not (record.get("archive") or {}).get("completeCode"):
        return {
            "problemNo": problem_no,
            "title": str(record["title"]),
            "source": None,
            "candidateSha256": "",
            "sampleInputBytes": 0,
            "expectedOutputBytes": 0,
            "category": "TOPIC_ONLY",
            "compileDiagnostic": "",
            "runtimeDiagnostic": "题面已归档，但尚无本人 Accepted 源码；跳过样例编译与运行。",
        }
    statement = statement_path(record)
    sample_input, expected, extraction_error = extract_sample_pair(statement)
    sample_correction = ""
    if (
        not extraction_error
        and problem_no in SAMPLE_CORRECTIONS
        and sample_input.rstrip("\n").endswith("#Coding")
    ):
        sample_input = sample_input.rstrip("\n") + " \n"
        sample_correction = SAMPLE_CORRECTIONS[problem_no]
    sample_input, compacted_input = compact_html_formatting_blank_lines(sample_input)
    expected, compacted_output = compact_html_formatting_blank_lines(expected)
    result: dict[str, Any] = {
        "problemNo": problem_no,
        "title": str(record["title"]),
        "source": "",
        "candidateSha256": "",
        "sampleInputBytes": len(sample_input.encode("utf-8")),
        "expectedOutputBytes": len(expected.encode("utf-8")),
        "category": "NOT_RUN",
        "compileDiagnostic": "",
        "runtimeDiagnostic": "",
    }
    if compacted_input or compacted_output:
        result["sampleFormatting"] = "removed_blank_lines_from_converted_html_paragraphs"
    if sample_correction:
        result["sampleCorrection"] = sample_correction
    if extraction_error:
        result["category"] = "NO_SAMPLE"
        result["runtimeDiagnostic"] = extraction_error
        return result
    if problem_no in SAMPLE_QUALITY_ISSUES:
        result["category"] = "SAMPLE_INCONSISTENT"
        result["runtimeDiagnostic"] = SAMPLE_QUALITY_ISSUES[problem_no]
        return result
    if problem_no in KNOWN_FILL_IN_FRAGMENTS:
        result["category"] = "FILL_IN_FRAGMENT"
        result["runtimeDiagnostic"] = "固定模板不可得，不能为样例运行臆造完整程序"
        return result
    raw_source = ROOT / str(record["archive"]["completeCode"])
    source = candidate_path(raw_source)
    result["source"] = source.relative_to(ROOT).as_posix()
    result["candidateSha256"] = hashlib.sha256(source.read_bytes()).hexdigest()
    language = str(record.get("language") or "")
    if language.startswith("python"):
        command = [sys.executable, str(source)]
        if not run:
            result["category"] = "READY_TO_RUN"
            return result
        stdout, stderr, returncode, process_error = run_process(command, sample_input, 5)
    else:
        executable = WORK_ROOT / ("prog_" + hashlib.sha256(str(source).encode()).hexdigest()[:16])
        command = compile_command(record, source, executable)
        if command is None:
            result["category"] = "UNSUPPORTED_LANGUAGE"
            return result
        if not run:
            result["category"] = "READY_TO_RUN"
            return result
        stdout, stderr, returncode, process_error = run_process(command, "", 30)
        if process_error:
            result["category"] = "COMPILE_PROCESS_ERROR"
            result["compileDiagnostic"] = stderr
            return result
        if returncode != 0:
            result["category"] = "COMPILE_ERROR"
            result["compileDiagnostic"] = stderr.splitlines()[:12]
            return result
        stdout, stderr, returncode, process_error = run_process([str(executable)], sample_input, 5)
    result["runtimeDiagnostic"] = stderr.splitlines()[:12]
    if process_error:
        result["category"] = process_error
    elif returncode != 0:
        result["category"] = "RUNTIME_ERROR"
    else:
        matched, comparison = outputs_match(stdout, expected)
        result["comparison"] = comparison
        if matched:
            result["category"] = "PASS"
        else:
            result["category"] = "WRONG_OUTPUT"
            result["actualOutput"] = stdout
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="运行题面样例的离线回归测试")
    parser.add_argument("--check", action="store_true", help="只抽取样例，不编译或运行")
    parser.add_argument("--limit", type=int, default=0, help="仅运行前 N 条记录，0 表示全部")
    parser.add_argument("--problem", dest="problem_nos", action="append", type=int, help="只处理指定题号")
    args = parser.parse_args()

    records = json.loads(DATA_PATH.read_text(encoding="utf-8"))["records"]
    problem_filter = set(args.problem_nos or [])
    selected_records = [
        record for record in records if not problem_filter or int(record["problemNo"]) in problem_filter
    ]
    selected = selected_records if args.limit <= 0 else selected_records[: args.limit]
    if not args.check:
        WORK_ROOT.mkdir(parents=True, exist_ok=True)
    results = [test_record(record, run=not args.check) for record in selected]
    counts = Counter(item["category"] for item in results)
    report = {
        "schemaVersion": 1,
        "purpose": "offline statement sample regression; not an online AC proof",
        "python": sys.version,
        "platform": platform.platform(),
        "compiler": shutil.which("clang++") or shutil.which("clang") or "unavailable",
        "records": len(results),
        "counts": dict(sorted(counts.items())),
        "results": results,
        "notes": [
            "样例按题面 Markdown 的输入样例/输出样例段落抽取，并按空白分词比较；转换器从 HTML 生成的段落空行会被视为排版空白。",
            "含小数或科学计数法的样例允许 1e-3 绝对误差；这只改变离线样例分类，不代表在线 Accepted。",
            "无法可靠配对样例、填空片段和特殊题型不会被自动补全。",
            "SAMPLE_INCONSISTENT 表示捕获到的样例自身违反题面格式，不能据此修改代码；只有有明确题面规则支持的 1645 尾随空格会在输入副本中校正并留痕。",
            "样例通过不代表边界正确、分块回环通过、在线 Accepted 或 PUBLIC_READY。",
        ],
    }
    if not args.check:
        REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
        REPORT_PATH.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"records": len(results), "counts": dict(sorted(counts.items()))}, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
