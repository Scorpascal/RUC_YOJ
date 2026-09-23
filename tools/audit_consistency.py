#!/usr/bin/env python3
"""Fail-closed audit for the public YOJ release evidence chain."""

from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path
from typing import Any

try:
    from audit_online_availability import validate_snapshot
except ImportError:  # pragma: no cover - supports package-style imports
    from tools.audit_online_availability import validate_snapshot


ROOT = Path(__file__).resolve().parents[1]
STATEMENT_STATUS_SOURCE = "[data/problems.json](../../data/problems.json)"
DYNAMIC_STATEMENT_STATUS_MARKERS = (
    "> 当前仓库阶段：",
    "> 当前版本已完成",
    "## 归档状态",
    "- 代码状态：",
    "- 在线 AC 复验：",
    "- 直接提交分块：",
)


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
    quick_payload = load("data/quick-submit.json")
    docs_quick_payload = load("docs/data/quick-submit.json")
    if quick_payload.get("schemaVersion") != 2:
        failures.append("quick-submit schemaVersion must be 2 when archivedEntries is present")
    quick = index(quick_payload.get("entries") or [], "quick-submit", failures)
    docs_quick = index(docs_quick_payload.get("entries") or [], "docs quick-submit", failures)
    archived_quick = index(quick_payload.get("archivedEntries") or [], "archived quick-submit", failures)
    docs_archived_quick = index(
        docs_quick_payload.get("archivedEntries") or [], "docs archived quick-submit", failures
    )
    catalog = index(load("docs/data/catalog.json").get("entries") or [], "catalog", failures)
    online_payload = load("data/yoj-public-problems.json")
    online = index(validate_snapshot(online_payload), "YOJ public-index snapshot", failures)
    readme_path = ROOT / "README.md"
    readme = readme_path.read_text(encoding="utf-8") if readme_path.is_file() else ""

    current_online_skips = sum(
        1
        for problem in problems.values()
        if str((problem.get("public") or {}).get("onlineVerification") or "").startswith("ONLINE_SKIPPED_")
    )
    expected_skip_summary = (
        f"当前状态仍标记为在线跳过 `{current_online_skips}` 道"
        "（按 `data/problems.json` 统计，历史跳过记录不重复计数）"
    )
    if readme.count("当前状态仍标记为在线跳过 `") != 1 or expected_skip_summary not in readme:
        failures.append("README current online-skip count differs from data/problems.json")

    if set(catalog) != set(problems):
        failures.append("catalog problem-number set differs from data/problems.json")
    if set(quick) != set(ready):
        failures.append("quick-submit problem-number set differs from data/public-ready.json")
    if set(docs_quick) != set(quick):
        failures.append("docs quick-submit problem-number set differs from data/quick-submit.json")
    if set(docs_archived_quick) != set(archived_quick):
        failures.append("docs archived quick-submit problem-number set differs from data/quick-submit.json")
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
        public = problem.get("public") or {}
        statement_path = str(public.get("statement") or "")
        statement_file = ROOT / statement_path
        if not statement_path or not statement_file.is_file():
            failures.append(f"{number}: problem statement missing: {statement_path or '<empty>'}")
        else:
            statement_text = statement_file.read_text(encoding="utf-8")
            if STATEMENT_STATUS_SOURCE not in statement_text:
                failures.append(f"{number}: statement does not direct readers to the canonical status index")
            if any(marker in statement_text for marker in DYNAMIC_STATEMENT_STATUS_MARKERS):
                failures.append(f"{number}: statement contains a dynamic status projection")

        row_match = re.search(
            rf"^\| {number} \|.*\| `([^`]+)` \|$", readme, re.MULTILINE
        )
        if row_match is None:
            failures.append(f"{number}: README status row is missing")
        else:
            current_online_status = str(public.get("onlineVerification") or "NOT_RECORDED")
            readme_status = row_match.group(1)
            if f"; {current_online_status}" not in f"; {readme_status}":
                failures.append(f"{number}: README online status differs from data/problems.json")
            if current_online_status == "NO_LOCAL_AC" and "ONLINE_SKIPPED_" in readme_status:
                failures.append(f"{number}: README projects a historical skip as a current status")

        is_ready = (problem.get("public") or {}).get("status") == "PUBLIC_READY"
        if is_ready != (number in ready):
            failures.append(f"{number}: problems/public-ready status membership differs")
        if number in catalog and bool(catalog[number].get("verified")) != is_ready:
            failures.append(f"{number}: catalog verified flag differs from release status")

    for number, archived in archived_quick.items():
        if number in ready:
            failures.append(f"{number}: archived quick-submit overlaps PUBLIC_READY entry")
        if number not in problems or number not in online:
            failures.append(f"{number}: archived quick-submit is not a current local YOJ-public problem")
            continue
        if str(archived.get("source") or "") != "ACCEPTED_ARCHIVE":
            failures.append(f"{number}: archived quick-submit source is not ACCEPTED_ARCHIVE")
        code_path = str(archived.get("codePath") or "")
        check_file(number, "archived direct code", code_path, str(archived.get("codeSha256") or ""), failures)
        if str(archived.get("onlineStatus") or "") != "Accepted (历史归档)":
            failures.append(f"{number}: archived quick-submit status is not historical Accepted")
        catalog_row = catalog.get(number)
        if not catalog_row or catalog_row.get("quickSubmitMode") != "archived":
            failures.append(f"{number}: catalog missing archived quick-submit projection")
        elif str(catalog_row.get("codeUrl") or "") != str(archived.get("codeUrl") or ""):
            failures.append(f"{number}: catalog archived codeUrl differs from quick-submit")

    for number, catalog_row in catalog.items():
        expected_online = number in online
        if bool(catalog_row.get("onlineAvailable")) != expected_online:
            failures.append(f"{number}: catalog onlineAvailable differs from YOJ public-index snapshot")
        expected_status = "YOJ_PUBLIC" if expected_online else "NOT_IN_CURRENT_PUBLIC_INDEX"
        if str(catalog_row.get("onlineStatus") or "") != expected_status:
            failures.append(f"{number}: catalog onlineStatus differs from YOJ public-index snapshot")
        expected_online_accepted = str((problems[number].get("public") or {}).get("onlineVerification") or "") == "ONLINE_ACCEPTED"
        if bool(catalog_row.get("onlineAccepted")) != expected_online_accepted:
            failures.append(f"{number}: catalog onlineAccepted differs from data/problems.json")

    source = load("docs/data/catalog.json").get("source") or {}
    expected_online_ids = set(online)
    def source_count(key: str) -> int:
        # Zero is a valid count (for example, when no archived quick-submit
        # fallback exists).  Do not turn it into the missing-value sentinel.
        value = source.get(key)
        return int(value) if value is not None else -1

    if source_count("onlinePublicProblems") != len(expected_online_ids):
        failures.append("catalog source onlinePublicProblems differs from the snapshot")
    if str(source.get("onlineSnapshotCapturedAt") or "") != str(online_payload.get("capturedAt") or ""):
        failures.append("catalog source onlineSnapshotCapturedAt differs from the snapshot")
    if str(source.get("onlineSnapshotSha256") or "") != str(online_payload.get("problemListSha256") or ""):
        failures.append("catalog source onlineSnapshotSha256 differs from the snapshot")
    if source_count("archivedQuickSubmitEntries") != len(archived_quick):
        failures.append("catalog source archivedQuickSubmitEntries differs from the manifest")
    if source_count("archivedCodeEntries") != sum(bool(row.get("archiveCodeAvailable")) for row in catalog.values()):
        failures.append("catalog source archivedCodeEntries differs from the catalog")
    if source_count("onlineAcceptedSubmissions") != sum(bool(row.get("onlineAccepted")) for row in catalog.values()):
        failures.append("catalog source onlineAcceptedSubmissions differs from the catalog")

    online_missing_from_repository = sorted(set(online) - set(problems))
    if online_missing_from_repository:
        failures.append(
            "YOJ public-index problems are missing from local capture: "
            + ", ".join(str(number) for number in online_missing_from_repository)
        )

    result = {
        "problemRecords": len(problems),
        "publicReady": len(ready),
        "quickSubmit": len(quick),
        "archivedQuickSubmit": len(archived_quick),
        "catalogRecords": len(catalog),
        "catalogVerified": sum(bool(row.get("verified")) for row in catalog.values()),
        "onlinePublicProblems": len(online),
        "onlinePublicInRepository": len(set(online) & set(problems)),
        "onlinePublicMissingRepository": len(online_missing_from_repository),
        "repositoryProblemsNotInCurrentPublicIndex": len(set(problems) - set(online)),
        "failures": failures,
    }
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
