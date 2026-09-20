#!/usr/bin/env python3
"""Fail-closed audit for the public YOJ release evidence chain."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]


def load(relative: str) -> dict[str, Any]:
    return json.loads((ROOT / relative).read_text(encoding="utf-8"))


def sha256(relative: str) -> str:
    digest = hashlib.sha256()
    with (ROOT / relative).open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def index(rows: list[dict[str, Any]], label: str, failures: list[str]) -> dict[int, dict[str, Any]]:
    result: dict[int, dict[str, Any]] = {}
    for row in rows:
        number = int(row["problemNo"])
        if number in result:
            failures.append(f"{label}: duplicate problemNo {number}")
        result[number] = row
    return result


def check_file(number: int, label: str, path: str, expected: str, failures: list[str]) -> None:
    target = ROOT / path
    if not path or not target.is_file():
        failures.append(f"{number}: {label} missing: {path or '<empty>'}")
        return
    actual = sha256(path)
    if actual != expected:
        failures.append(f"{number}: {label} SHA mismatch: expected {expected}, got {actual}")


def main() -> int:
    failures: list[str] = []
    problems = index(load("data/problems.json").get("records") or [], "problems", failures)
    ready = index(load("data/public-ready.json").get("records") or [], "public-ready", failures)
    quick = index(load("data/quick-submit.json").get("entries") or [], "quick-submit", failures)
    docs_quick = index(load("docs/data/quick-submit.json").get("entries") or [], "docs quick-submit", failures)
    catalog = index(load("docs/data/catalog.json").get("entries") or [], "catalog", failures)

    if set(catalog) != set(problems):
        failures.append("catalog problem-number set differs from data/problems.json")
    if set(quick) != set(ready):
        failures.append("quick-submit problem-number set differs from data/public-ready.json")
    if set(docs_quick) != set(quick):
        failures.append("docs quick-submit problem-number set differs from data/quick-submit.json")
    if (ROOT / "data/quick-submit.json").read_bytes() != (ROOT / "docs/data/quick-submit.json").read_bytes():
        failures.append("the two quick-submit manifests are not byte-identical")

    for number, release in ready.items():
        problem = problems.get(number)
        submit = quick.get(number)
        catalog_row = catalog.get(number)
        if not problem or not submit or not catalog_row:
            continue
        public = problem.get("public") or {}
        complete_path = str(release.get("completeCode") or "")
        direct_path = str(release.get("directlySubmittableCode") or "")
        statement_path = str(public.get("statement") or "")
        check_file(number, "complete code", complete_path, str(release.get("completeCodeSha256") or ""), failures)
        check_file(number, "direct code", direct_path, str(release.get("directlySubmittableCodeSha256") or ""), failures)
        check_file(number, "statement", statement_path, str(release.get("statementSha256") or ""), failures)

        expected = {
            "status": "PUBLIC_READY",
            "cleanCode": complete_path,
            "directlySubmittableCode": direct_path,
            "verifiedSubmissionNo": str(release.get("submissionNo") or ""),
            "codeSha256": str(release.get("directlySubmittableCodeSha256") or ""),
        }
        for key, value in expected.items():
            if str(public.get(key) or "") != value:
                failures.append(f"{number}: problems.public.{key} differs from public-ready")

        quick_expected = {
            "codePath": direct_path,
            "codeSha256": str(release.get("directlySubmittableCodeSha256") or ""),
            "onlineStatus": "Accepted",
            "onlineSubmissionNo": str(release.get("submissionNo") or ""),
            "language": str(release.get("language") or ""),
        }
        for key, value in quick_expected.items():
            if str(submit.get(key) or "") != value:
                failures.append(f"{number}: quick-submit.{key} differs from public-ready")
        if catalog_row.get("verified") is not True:
            failures.append(f"{number}: catalog is not verified")
        if str(catalog_row.get("submissionNo") or "") != str(release.get("submissionNo") or ""):
            failures.append(f"{number}: catalog submissionNo differs from public-ready")
        if str(catalog_row.get("language") or "") != str(release.get("language") or ""):
            failures.append(f"{number}: catalog language differs from public-ready")
        if str(catalog_row.get("codeUrl") or "") != str(submit.get("codeUrl") or ""):
            failures.append(f"{number}: catalog codeUrl differs from quick-submit")

    for number, problem in problems.items():
        is_ready = (problem.get("public") or {}).get("status") == "PUBLIC_READY"
        if is_ready != (number in ready):
            failures.append(f"{number}: problems/public-ready status membership differs")
        if number in catalog and bool(catalog[number].get("verified")) != is_ready:
            failures.append(f"{number}: catalog verified flag differs from release status")

    result = {
        "problemRecords": len(problems),
        "publicReady": len(ready),
        "quickSubmit": len(quick),
        "catalogRecords": len(catalog),
        "catalogVerified": sum(bool(row.get("verified")) for row in catalog.values()),
        "failures": failures,
    }
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
