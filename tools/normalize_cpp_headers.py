#!/usr/bin/env python3
"""Create language-compatible C++ candidates without changing raw captures.

The raw ``代码库/`` is evidence and remains byte-for-byte untouched.  This
tool replaces ``#include <bits/stdc++.h>`` only in a generated candidate tree
under ``staging/``.  The candidate still requires the normal cleaning,
compilation, online verification, and publication gates.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
RAW_ROOT = ROOT / "代码库"
DEFAULT_OUTPUT = ROOT / "staging" / "cpp17-compatible"

CPP_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
BITS_INCLUDE = re.compile(r"(?m)^[ \t]*#\s*include\s*[<\"]bits/stdc\+\+\.h[>\"]\s*\r?\n?")

# A deliberately portable C++17 set.  It avoids compiler-internal umbrella
# headers while covering the standard-library facilities commonly used by the
# captured solutions whose YOJ language is explicitly C++11 or newer.
PORTABLE_HEADERS = """#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <vector>
"""

# The plain YOJ ``cpp`` form is an older GNU C++ mode.  It accepts the GNU
# umbrella header in the original submission, but a replacement bundle must
# not introduce C++11/17-only headers such as <array>, <chrono>, <random>, or
# <unordered_map>.  Keep this profile C++98-compatible; the local C++17 gate
# can still compile it, while the online ``cpp`` compiler can accept it.
CPP98_HEADERS = """#include <algorithm>
#include <bitset>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <utility>
#include <valarray>
#include <vector>
"""


def header_bundle_for_path(path: Path) -> tuple[str, str]:
    """Choose a bundle from the language encoded in a captured filename."""

    name = path.name.lower()
    if re.search(r"_cpp_", name):
        return CPP98_HEADERS, "C++98-compatible headers for YOJ cpp"
    return PORTABLE_HEADERS, "portable C++11+ standard headers"


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def normalize_file(source: Path, target: Path, profile: str = "auto") -> dict[str, Any]:
    original = source.read_bytes()
    text = original.decode("utf-8")
    matches = list(BITS_INCLUDE.finditer(text))
    if profile == "cpp98":
        headers, header_profile = CPP98_HEADERS, "C++98-compatible headers for YOJ cpp"
    elif profile == "cpp17":
        headers, header_profile = PORTABLE_HEADERS, "portable C++11+ standard headers"
    else:
        headers, header_profile = header_bundle_for_path(source)
    normalized = BITS_INCLUDE.sub(headers, text)
    output = normalized.encode("utf-8")
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(output)
    return {
        "source": source.relative_to(ROOT).as_posix(),
        "candidate": target.relative_to(ROOT).as_posix(),
        "replacements": len(matches),
        "sourceSha256": sha256_bytes(original),
        "candidateSha256": sha256_bytes(output),
        "sourceBytes": len(original),
        "candidateBytes": len(output),
        "headerProfile": header_profile,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="在 staging 中生成不依赖 bits/stdc++.h 的语言兼容 C++ 候选")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT, help="候选输出目录，默认在被忽略的 staging/ 下")
    parser.add_argument("--check", action="store_true", help="只统计命中，不生成候选文件")
    parser.add_argument(
        "--profile",
        choices=("auto", "cpp98", "cpp17"),
        default="auto",
        help="头文件配置；默认按文件名中的 YOJ 语言自动选择",
    )
    parser.add_argument(
        "--problem",
        dest="problem_nos",
        action="append",
        type=int,
        help="只重建指定题号，可重复传入；用于隔离在线调试候选",
    )
    args = parser.parse_args()

    output_root = args.output if args.output.is_absolute() else ROOT / args.output
    selected = set(args.problem_nos or [])
    candidates: list[Path] = []
    matched_replacements = 0
    for path in sorted(RAW_ROOT.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in CPP_SUFFIXES:
            continue
        if selected:
            match = re.match(r"^(\d{4})_", path.parent.name)
            if not match or int(match.group(1)) not in selected:
                continue
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        match_count = len(BITS_INCLUDE.findall(text))
        if match_count:
            candidates.append(path)
            matched_replacements += match_count

    report_files: list[dict[str, Any]] = []
    failures: list[dict[str, str]] = []
    if not args.check:
        for source in candidates:
            target = output_root / source.relative_to(RAW_ROOT)
            try:
                report_files.append(normalize_file(source, target, args.profile))
            except (OSError, UnicodeDecodeError) as exc:
                failures.append({"source": source.relative_to(ROOT).as_posix(), "error": f"{exc.__class__.__name__}: {exc}"})
        write_json(
            output_root.parent / "cpp17-header-normalization-report.json",
            {
                "schemaVersion": 1,
                "purpose": "private candidate transformation; raw archive unchanged",
                "rule": "replace bits/stdc++.h with a language-compatible standard-header profile",
                "sourceRoot": RAW_ROOT.relative_to(ROOT).as_posix(),
                "candidateRoot": output_root.relative_to(ROOT).as_posix(),
                "filesMatched": len(candidates),
                "filesWritten": len(report_files),
                "replacementCount": sum(item["replacements"] for item in report_files),
                "failures": failures,
                "files": report_files,
            },
        )

    result = {
        "filesMatched": len(candidates),
        "filesWritten": len(report_files),
        "replacementCount": sum(item["replacements"] for item in report_files) if not args.check else matched_replacements,
        "failures": failures,
        "rawArchiveChangedByThisTool": False,
    }
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
