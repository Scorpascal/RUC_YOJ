#!/usr/bin/env python3
"""Audit observed YOJ submission forms without sending a request."""

from __future__ import annotations

import argparse
import hashlib
import json
from collections import Counter
from pathlib import Path
from typing import Any

from lxml import html


ROOT = Path(__file__).resolve().parents[1]
RAW_ROOT = ROOT / "代码库"
DATA_PATH = ROOT / "data" / "problems.json"
REPORT_PATH = ROOT / "staging" / "form-audit.json"


def parse_snapshot(path: Path) -> Any:
    raw = path.read_bytes()
    for encoding in ("utf-8", "gb18030", "big5"):
        try:
            return html.fromstring(raw.decode(encoding))
        except UnicodeDecodeError:
            continue
    return html.fromstring(raw.decode("utf-8", errors="replace"))


def signature_for(form: Any, document: Any) -> dict[str, Any]:
    hidden = []
    for node in form.xpath('.//input[@type="hidden"]'):
        name = node.get("name") or ""
        value = node.get("value") or ""
        # pid is a per-problem value; retain the field but not its changing ID.
        if name == "pid":
            value = "<problem_no>"
        hidden.append((name, node.get("id") or "", value))
    languages = []
    for node in form.xpath('.//*[contains(concat(" ", normalize-space(@class), " "), " item ")]'):
        languages.append(
            (
                node.get("data-value") or "",
                node.get("data-mode") or "",
                "active" in (node.get("class") or "").split(),
            )
        )
    return {
        "action": form.get("action") or "",
        "method": (form.get("method") or "").lower(),
        "enctype": form.get("enctype") or "",
        "hidden": sorted(hidden),
        "editorCount": len(document.xpath('//*[@id="editor"]')),
        "fileInputCount": len(form.xpath('.//input[@type="file"]')),
        "languageOptions": languages,
        "activeLanguage": [item for item in languages if item[2]],
        "contenteditableCount": len(document.xpath('//*[@contenteditable="true"]')),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="离线审计 YOJ 提交表单契约")
    parser.add_argument("--check", action="store_true", help="只运行并打印检查，不写报告")
    parser.add_argument("--problem", dest="problem_nos", action="append", type=int, help="只处理指定题号")
    args = parser.parse_args()

    records = json.loads(DATA_PATH.read_text(encoding="utf-8"))["records"]
    selected = set(args.problem_nos or [])
    rows = []
    signatures = Counter()
    mismatches = []
    missing_detail_snapshots = 0
    for record in records:
        if selected and int(record["problemNo"]) not in selected:
            continue
        archive = record.get("archive") or {}
        if not archive.get("completeCode"):
            rows.append(
                {
                    "problemNo": record["problemNo"],
                    "title": record["title"],
                    "detail": None,
                    "signatureHash": None,
                    "signature": None,
                    "archivedLanguage": str(record.get("language") or ""),
                    "archivedLanguageAvailable": False,
                    "classification": "TOPIC_ONLY_NO_LOCAL_AC",
                }
            )
            continue
        raw_path = ROOT / str(archive["completeCode"])
        folder = RAW_ROOT / str(record["folder"])
        submission_no = str(archive.get("submissionNo") or "")
        fallback_paths = [
            folder / f"{int(record['problemNo']):04d}_提交_{submission_no}_详情.html",
            folder / (raw_path.name.split("_完整代码", 1)[0] + "_详情.html"),
        ]
        # The metadata is authoritative for the detail HTML filename.
        metadata_files = sorted(folder.glob("*_元数据.json"))
        metadata = json.loads(metadata_files[0].read_text(encoding="utf-8")) if metadata_files else {}
        detail_name = str(
            (metadata.get("files") or {}).get("submissionHtml")
            or (metadata.get("files") or {}).get("submission")
            or ""
        )
        candidates = ([folder / detail_name] if detail_name else []) + fallback_paths
        # Some older local captures kept the language in the detail filename;
        # accept that historical form as a final local-only fallback.
        if submission_no:
            candidates.extend(sorted(folder.glob(f"{int(record['problemNo']):04d}_提交_{submission_no}_*_详情.html")))
        detail_path = next((candidate for candidate in candidates if candidate.is_file()), candidates[0])
        if not detail_path.is_file():
            # Submission detail HTML is intentionally ignored by the public
            # repository. A fresh checkout therefore cannot always provide
            # this optional local observation. Record the gap and let
            # online_verify perform its live form gate instead of crashing the
            # whole daily sync.
            missing_detail_snapshots += 1
            rows.append(
                {
                    "problemNo": record["problemNo"],
                    "title": record["title"],
                    "detail": detail_path.relative_to(ROOT).as_posix(),
                    "signatureHash": None,
                    "signature": None,
                    "archivedLanguage": str(record.get("language") or ""),
                    "archivedLanguageAvailable": None,
                    "classification": "DETAIL_SNAPSHOT_MISSING",
                }
            )
            continue
        document = parse_snapshot(detail_path)
        forms = document.xpath('//form[@id="submit_code"]')
        form = forms[0] if forms else None
        signature = signature_for(form, document) if form is not None else {"form": "missing"}
        signature_json = json.dumps(signature, ensure_ascii=False, sort_keys=True)
        signature_hash = hashlib.sha256(signature_json.encode("utf-8")).hexdigest()
        signatures[signature_hash] += 1
        hidden_names = {item[0] for item in signature.get("hidden", [])}
        active = signature.get("activeLanguage", [])
        expected_language = str(record.get("language") or "")
        if form is None or signature.get("editorCount") != 1 or not {"code", "language", "pid"}.issubset(hidden_names):
            mismatches.append({"problemNo": record["problemNo"], "reason": "form_shape_not_observed"})
        available_languages = {item[0] for item in signature.get("languageOptions", [])}
        if expected_language not in available_languages:
            mismatches.append(
                {
                    "problemNo": record["problemNo"],
                    "reason": "archived_language_not_in_form_options",
                    "available": sorted(available_languages),
                    "archived": expected_language,
                }
            )
        rows.append(
            {
                "problemNo": record["problemNo"],
                "title": record["title"],
                "detail": detail_path.relative_to(ROOT).as_posix(),
                "signatureHash": signature_hash,
                "signature": signature,
                "archivedLanguage": expected_language,
                "archivedLanguageAvailable": expected_language in available_languages,
                "classification": "FORM_OBSERVED_ONE_EDITOR_HIDDEN_CODE" if form is not None else "FORM_NOT_FOUND",
            }
        )

    report = {
        "schemaVersion": 1,
        "purpose": "offline observation only; no submission request sent",
        "records": len(rows),
        "uniqueFormSignatures": len(signatures),
        "signatureCounts": dict(signatures),
        "mismatches": mismatches,
        "missingDetailSnapshots": missing_detail_snapshots,
        "rows": rows,
        "nextGate": "FORM_MAPPED requires one controlled dry-run and round-trip verification; missing local detail snapshots are checked by online_verify against the live page",
    }
    if not args.check:
        REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
        REPORT_PATH.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(
        json.dumps(
            {
                "records": len(rows),
                "uniqueFormSignatures": len(signatures),
                "mismatches": len(mismatches),
                "missingDetailSnapshots": missing_detail_snapshots,
            },
            ensure_ascii=False,
            indent=2,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
