"""Private, bounded evidence of source access and completed nightly work.

Only fixed enums and timestamps are stored. This is independent of content
timestamps and receipt delivery; neither unchanged data nor a replay is a new
successful sync by itself.
"""

from __future__ import annotations

import json
import fcntl
import os
import re
import tempfile
import time
from datetime import datetime
from pathlib import Path

SOURCE_STATES = {"UNKNOWN", "AVAILABLE", "BLOCKED", "TRANSIENT_ERROR", "ERROR"}
SYNC_STATES = {"SUCCEEDED", "INCOMPLETE", "BLOCKED", "SKIPPED", "FAILED"}
BLOCKING_REASONS = {"YOJ_TLS_CERTIFICATE_ERROR", "YOJ_CAMPUS_ACCESS_REQUIRED", "YOJ_AUTH_REQUIRED"}
REASONS = BLOCKING_REASONS | {
    "NONE", "YOJ_TRANSIENT_NETWORK_ERROR", "YOJ_UNEXPECTED_DESTINATION",
    "YOJ_HTTPS_DOWNGRADE_REFUSED", "YOJ_INVALID_PUBLIC_INDEX", "YOJ_SOURCE_ERROR",
    "SYNC_INCOMPLETE", "SYNC_FAILED", "SYNC_SKIPPED", "ACCESS_COOLDOWN",
}
STATUS_KEYS = {"schemaVersion", "startedAt", "finishedAt", "mode", "sourceState",
               "syncState", "reason", "lastSuccessfulSyncAt", "retryAfter"}
STAMP_RE = re.compile(r"\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\+08:00\Z")
INVOCATION_RE = re.compile(r"[0-9a-f]{32}\Z")
MAX_BYTES = 8192


def timestamp(value: object) -> datetime:
    if not isinstance(value, str) or not STAMP_RE.fullmatch(value):
        raise ValueError("INVALID_SYNC_STATUS")
    return datetime.fromisoformat(value)


def validate(payload: object) -> dict:
    if not isinstance(payload, dict) or set(payload) not in (STATUS_KEYS, STATUS_KEYS | {"invocationId"}):
        raise ValueError("INVALID_SYNC_STATUS")
    if "invocationId" in payload and (not isinstance(payload["invocationId"], str)
                                      or not INVOCATION_RE.fullmatch(payload["invocationId"])):
        raise ValueError("INVALID_SYNC_STATUS")
    if type(payload["schemaVersion"]) is not int or payload["schemaVersion"] != 1:
        raise ValueError("INVALID_SYNC_STATUS")
    started, finished = timestamp(payload["startedAt"]), timestamp(payload["finishedAt"])
    if (started > finished or payload["mode"] not in {"MAIN", "VISIBILITY"}
            or payload["sourceState"] not in SOURCE_STATES or payload["syncState"] not in SYNC_STATES
            or payload["reason"] not in REASONS):
        raise ValueError("INVALID_SYNC_STATUS")
    last = payload["lastSuccessfulSyncAt"]
    if last is not None and timestamp(last) > finished:
        raise ValueError("INVALID_SYNC_STATUS")
    retry = payload["retryAfter"]
    if retry is not None:
        timestamp(retry)
        if payload["sourceState"] != "BLOCKED" or payload["reason"] not in BLOCKING_REASONS:
            raise ValueError("INVALID_SYNC_STATUS")
    if payload["syncState"] == "SUCCEEDED":
        if payload["sourceState"] != "AVAILABLE" or payload["reason"] != "NONE":
            raise ValueError("INVALID_SYNC_STATUS")
        if payload["mode"] == "MAIN" and last != payload["finishedAt"]:
            raise ValueError("INVALID_SYNC_STATUS")
    return payload


def read(state_dir: Path) -> dict | None:
    path = state_dir / "sync-status.json"
    try:
        if path.is_symlink() or not path.is_file():
            return None
        with path.open("rb") as handle:
            raw = handle.read(MAX_BYTES + 1)
        if len(raw) > MAX_BYTES:
            return None
        return validate(json.loads(raw))
    except (OSError, TypeError, ValueError):
        return None


def write(state_dir: Path, payload: dict) -> None:
    validate(payload)
    # Skipped admission is already recorded in the scheduler receipt. It must
    # not overwrite a concurrent run's actual business result or success time.
    if payload["syncState"] == "SKIPPED":
        return
    state_dir.mkdir(parents=True, exist_ok=True)
    with (state_dir / "sync-status.lock").open("a") as lock:
        deadline = time.monotonic() + 1
        while True:
            try:
                fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
                break
            except BlockingIOError:
                if time.monotonic() >= deadline:
                    raise OSError("SYNC_STATUS_BUSY") from None
                time.sleep(0.01)
        current = read(state_dir)
        if current and timestamp(current["finishedAt"]) > timestamp(payload["finishedAt"]):
            return  # A slower writer cannot replace a newer completed result.
        payload = dict(payload)
        if current and current["lastSuccessfulSyncAt"]:
            if (payload["lastSuccessfulSyncAt"] is None
                    or timestamp(current["lastSuccessfulSyncAt"]) > timestamp(payload["lastSuccessfulSyncAt"])):
                payload["lastSuccessfulSyncAt"] = current["lastSuccessfulSyncAt"]
        validate(payload)
        _replace(state_dir, payload)


def _replace(state_dir: Path, payload: dict) -> None:
    path = state_dir / "sync-status.json"
    if path.is_symlink():
        raise OSError("INVALID_SYNC_STATUS")
    fd, name = tempfile.mkstemp(prefix=".sync-status-", dir=state_dir)
    temporary = Path(name)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as handle:
            json.dump(payload, handle, sort_keys=True, ensure_ascii=True)
            handle.write("\n")
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)
