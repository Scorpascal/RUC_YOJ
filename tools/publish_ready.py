#!/usr/bin/env python3
"""Materialize only fully verified candidates into the public archive.

The source archive is collected before verification, but this command is the
only step allowed to replace public code with a cleaned candidate.  It keeps a
local backup under ``.yoj-sync/`` and writes a small public manifest containing
hashes and verification evidence, never credentials or session data.
"""

from __future__ import annotations

import argparse
import json
import shutil
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

try:
    from release_gate import ROOT, candidate_paths, is_public_ready, relative, sha256_file
except ImportError:  # pragma: no cover - supports ``import tools.publish_ready``
    from tools.release_gate import ROOT, candidate_paths, is_public_ready, relative, sha256_file


DATA_PATH = ROOT / "data" / "problems.json"
ONLINE_PATH = ROOT / "staging" / "online-verification.json"
PUBLIC_READY_PATH = DATA_PATH.with_name("public-ready.json")
BACKUP_ROOT = ROOT / ".yoj-sync" / "raw-before-public"


def utc_now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def load_json(path: Path, fallback: dict[str, Any] | None = None) -> dict[str, Any]:
    if not path.is_file():
        return fallback or {}
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, TypeError, ValueError, json.JSONDecodeError):
        return fallback or {}


def atomic_write(path: Path, value: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_bytes(value)
    temporary.replace(path)


def load_previous() -> dict[str, dict[str, Any]]:
    payload = load_json(PUBLIC_READY_PATH)
    return {
        str(item.get("problemNo")): item
        for item in payload.get("records", [])
        if item.get("problemNo") is not None and item.get("status") == "PUBLIC_READY"
    }


def load_online() -> dict[str, dict[str, Any]]:
    payload = load_json(ONLINE_PATH)
    return {
        str(item.get("problemNo")): item
        for item in payload.get("records", [])
        if item.get("problemNo") is not None
    }


def statement_hash(record: dict[str, Any]) -> str:
    statement = ROOT / str((record.get("public") or {}).get("statement") or "")
    return sha256_file(statement) if statement.is_file() else ""


def previous_release_intact(previous: dict[str, Any], current_statement_hash: str) -> bool:
    """Return whether an existing release still matches its own frozen files."""

    complete = ROOT / str(previous.get("completeCode") or "")
    direct = ROOT / str(previous.get("directlySubmittableCode") or "")
    return bool(
        complete.is_file()
        and direct.is_file()
        and sha256_file(complete) == str(previous.get("completeCodeSha256") or "")
        and sha256_file(direct) == str(previous.get("directlySubmittableCodeSha256") or "")
        and current_statement_hash == str(previous.get("statementSha256") or "")
    )


def ready_record(record: dict[str, Any], online: dict[str, Any], previous: dict[str, Any] | None) -> tuple[dict[str, Any] | None, list[str]]:
    problem_no = int(record["problemNo"])
    complete, direct = candidate_paths(record)
    ready, reasons = is_public_ready(online, complete, direct, problem_no)
    complete_sha = sha256_file(complete) if complete.is_file() else ""
    direct_sha = sha256_file(direct) if direct.is_file() else ""
    current_statement_hash = statement_hash(record)

    # PUBLIC_READY is immutable by the unattended pipeline.  A later AC or a
    # candidate at a newer archive path does not replace an intact frozen row;
    # replacing it requires a separately reviewed migration of the manifest.
    if previous and previous_release_intact(previous, current_statement_hash):
        return previous, []
    if previous and not ready:
        return None, reasons
    if not ready:
        return None, reasons

    value = {
        "status": "PUBLIC_READY",
        "problemNo": problem_no,
        "title": str(record.get("title") or ""),
        "completeCode": str((record.get("archive") or {}).get("completeCode") or ""),
        "directlySubmittableCode": str((record.get("archive") or {}).get("directlySubmittableCode") or ""),
        "completeCodeSha256": complete_sha,
        "directlySubmittableCodeSha256": direct_sha,
        "statementSha256": current_statement_hash,
        "candidatePath": str(online.get("candidatePath") or ""),
        "candidateSha256": str(online.get("candidateSha256") or ""),
        "submissionNo": int(online.get("submissionNo") or 0),
        "language": str(online.get("language") or record.get("language") or ""),
        "verifiedAt": str(online.get("verifiedAt") or ""),
        "roundTrip": {
            "status": str((online.get("roundTrip") or {}).get("status") or ""),
            "exactByteMatch": bool((online.get("roundTrip") or {}).get("exactByteMatch")),
            "canonicalByteMatch": bool((online.get("roundTrip") or {}).get("canonicalByteMatch")),
        },
    }
    if previous and previous.get("completeCodeSha256") == complete_sha and previous.get("directlySubmittableCodeSha256") == direct_sha:
        value["readyAt"] = str(previous.get("readyAt") or utc_now())
    else:
        value["readyAt"] = utc_now()
    return value, []


def backup_then_replace(source: Path, target: Path, backup_root: Path, relative_path: str) -> bool:
    if not source.is_file():
        return False
    if target.is_file() and target.read_bytes() == source.read_bytes():
        return False
    if target.is_file():
        backup = backup_root / relative_path
        backup.parent.mkdir(parents=True, exist_ok=True)
        if not backup.exists():
            shutil.copy2(target, backup)
    target.parent.mkdir(parents=True, exist_ok=True)
    temporary = target.with_name(target.name + ".tmp")
    shutil.copyfile(source, temporary)
    temporary.replace(target)
    return True


def update_metadata(record: dict[str, Any], ready: dict[str, Any], batch_id: str) -> bool:
    archive = record.get("archive") or {}
    metadata_ref = str(archive.get("metadata") or "")
    if not metadata_ref:
        folder = str(record.get("folder") or "")
        metadata_ref = f"代码库/{folder}/{int(record['problemNo']):04d}_元数据.json"
    metadata_path = ROOT / metadata_ref
    if not metadata_path.is_file():
        return False
    metadata = load_json(metadata_path)
    code = metadata.setdefault("code", {})
    if "rawArchiveSha256" not in code and code.get("sha256"):
        code["rawArchiveSha256"] = code["sha256"]
    code["sha256"] = ready["completeCodeSha256"]
    code["directlySubmittableSha256"] = ready["directlySubmittableCodeSha256"]
    code["status"] = "CLEANED_ONLINE_VERIFIED"
    previous_publication = metadata.get("publication") or {}
    publication = {
        "status": "PUBLIC_READY",
        "batchId": batch_id,
        "candidateSha256": ready["candidateSha256"],
        "submissionNo": ready["submissionNo"],
        "verifiedAt": ready["verifiedAt"],
    }
    if (
        previous_publication.get("status") == publication["status"]
        and previous_publication.get("candidateSha256") == publication["candidateSha256"]
        and int(previous_publication.get("submissionNo") or 0) == int(publication["submissionNo"] or 0)
        and previous_publication.get("verifiedAt") == publication["verifiedAt"]
    ):
        publication["batchId"] = str(previous_publication.get("batchId") or batch_id)
    metadata["publication"] = publication
    encoded = (json.dumps(metadata, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
    if metadata_path.read_bytes() == encoded:
        return False
    atomic_write(metadata_path, encoded)
    return True


def apply_release(records: list[dict[str, Any]], ready_by_no: dict[str, dict[str, Any]], batch_id: str) -> int:
    changed = 0
    backup_root = BACKUP_ROOT / batch_id
    previous_by_no = load_previous()
    for record in records:
        key = str(record["problemNo"])
        ready = ready_by_no.get(key)
        if not ready:
            continue
        # An unchanged previous manifest row is already materialized.  Do not
        # refresh archive metadata or paths merely because staging still holds
        # a matching candidate or a later AC record.
        if ready == previous_by_no.get(key):
            continue
        complete_candidate, direct_candidate = candidate_paths(record)
        # ``ready`` may be an intact previous release retained because the
        # current staging candidate has not completed a new online gate.  In
        # that case the candidate must not overwrite the frozen public bytes.
        candidate_matches_release = bool(
            complete_candidate.is_file()
            and direct_candidate.is_file()
            and sha256_file(complete_candidate) == str(ready.get("completeCodeSha256") or "")
            and sha256_file(direct_candidate) == str(ready.get("directlySubmittableCodeSha256") or "")
        )
        if not candidate_matches_release:
            continue
        archive = record.get("archive") or {}
        complete_target = ROOT / str(archive.get("completeCode") or "")
        direct_target = ROOT / str(archive.get("directlySubmittableCode") or "")
        changed += int(backup_then_replace(complete_candidate, complete_target, backup_root, relative(complete_target)))
        changed += int(backup_then_replace(direct_candidate, direct_target, backup_root, relative(direct_target)))
        changed += int(update_metadata(record, ready, batch_id))
    payload = {
        "schemaVersion": 1,
        "purpose": "public release manifest; credentials and session data are excluded",
        "updatedAt": utc_now(),
        "records": [ready_by_no[key] for key in sorted(ready_by_no, key=lambda item: int(item))],
    }
    encoded = (json.dumps(payload, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
    previous_payload = load_json(PUBLIC_READY_PATH)
    previous_records = previous_payload.get("records") or []
    if previous_records != payload["records"]:
        atomic_write(PUBLIC_READY_PATH, encoded)
        changed += 1
    return changed


def main() -> int:
    parser = argparse.ArgumentParser(description="只把通过本地与 YOJ 复核的清洗候选物化为公开版本")
    parser.add_argument("--check", action="store_true", help="只生成资格摘要，不改动公开文件")
    parser.add_argument("--apply", action="store_true", help="备份原文件后应用 PUBLIC_READY 版本")
    parser.add_argument("--batch-id", default=datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ"))
    args = parser.parse_args()
    if args.check and args.apply:
        raise SystemExit("--check 与 --apply 不能同时使用")

    data = load_json(DATA_PATH)
    records = list(data.get("records") or [])
    online = load_online()
    previous = load_previous()
    ready_by_no: dict[str, dict[str, Any]] = {}
    blocked: list[dict[str, Any]] = []
    for record in records:
        key = str(record.get("problemNo"))
        ready, reasons = ready_record(record, online.get(key, {}), previous.get(key))
        if ready:
            ready_by_no[key] = ready
        elif online.get(key):
            blocked.append({"problemNo": int(record["problemNo"]), "reasons": reasons})

    print(
        json.dumps(
            {
                "records": len(records),
                "publicReady": len(ready_by_no),
                "blockedWithOnlineEvidence": len(blocked),
                "blocked": blocked[:40],
                "apply": bool(args.apply),
            },
            ensure_ascii=False,
            indent=2,
        )
    )
    if args.apply:
        changed = apply_release(records, ready_by_no, args.batch_id)
        print(json.dumps({"changedFiles": changed, "manifest": relative(PUBLIC_READY_PATH)}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
