#!/usr/bin/env python3
"""Observe nightly execution without granting access to the publication path.

The local journal and isolated Git store are ignored. Only schema-validated
receipts leave this machine. Hooks never propagate errors into the scheduler;
network delivery runs in a separate, bounded process. The cloud checker is
read-only and does not import any YOJ login or submission code.
"""

from __future__ import annotations

import argparse
import base64
import fcntl
import json
import os
import re
import signal
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request
import uuid
from contextlib import contextmanager
from datetime import date, datetime, time as day_time, timedelta
from pathlib import Path
from zoneinfo import ZoneInfo

TZ = ZoneInfo("Asia/Shanghai")
REPOSITORY = "Scorpascal/RUC_YOJ"
REMOTE = f"https://github.com/{REPOSITORY}.git"
BRANCH = "codex/yoj-health"
REF = f"refs/heads/{BRANCH}"
# First complete nightly window after installation; no historical runs invented.
ENABLED_FROM = "2026-10-03"
AUTHOR = "Scorpascal"
EMAIL = "229578631+Scorpascal@users.noreply.github.com"
CONTEXT_ENV = "YOJ_HEALTH_CONTEXT"
MAX_BYTES = 8192
DELIVERY_BUDGET = 60
GIT_STEP_TIMEOUT = 20
DELIVERY_ERRORS = {"DELIVERY_TIMEOUT", "GIT_DELIVERY_FAILED", "RECEIPT_CONFLICT",
                   "INVALID_RECEIPT", "INVALID_REMOTE", "INVALID_REMOTE_TREE",
                   "LOCAL_STATE_ERROR"}
ATTEMPT_KEYS = {"attemptNo", "invokedAt", "enteredAt", "finishedAt", "outcome", "exitCode", "skipReason"}
SYNC_KEYS = {"sourceState", "syncState", "reason", "lastSuccessfulSyncAt"}
SOURCE_STATES = {"UNKNOWN", "AVAILABLE", "BLOCKED", "TRANSIENT_ERROR", "ERROR"}
SYNC_STATES = {"SUCCEEDED", "INCOMPLETE", "BLOCKED", "SKIPPED", "FAILED"}
SYNC_REASONS = {"NONE", "YOJ_TLS_CERTIFICATE_ERROR", "YOJ_CAMPUS_ACCESS_REQUIRED",
                "YOJ_AUTH_REQUIRED", "YOJ_TRANSIENT_NETWORK_ERROR", "YOJ_UNEXPECTED_DESTINATION",
                "YOJ_HTTPS_DOWNGRADE_REFUSED", "YOJ_INVALID_PUBLIC_INDEX", "YOJ_SOURCE_ERROR",
                "SYNC_INCOMPLETE", "SYNC_FAILED", "SYNC_SKIPPED", "ACCESS_COOLDOWN"}
DATE_RE = re.compile(r"\d{4}-\d{2}-\d{2}\Z")
TOKEN_RE = re.compile(r"[0-9a-f]{32}\Z")
SHA_RE = re.compile(r"[0-9a-f]{40}\Z")
PATH_RE = re.compile(r"receipts/(\d{4}-\d{2}-\d{2})\.json\Z")


class HealthError(Exception):
    """Only fixed error categories may reach public output."""


class GitCommandError(HealthError):
    """Private delivery diagnostics; never retain command arguments or stderr."""

    def __init__(self, category: str, step: str, failure: str):
        super().__init__(category)
        self.step = step if step in {
            "init", "ls-remote", "fetch", "rev-parse", "ls-tree", "cat-file",
            "hash-object", "mktree", "commit-tree", "push",
        } else "OTHER"
        self.failure = failure if failure in {
            "AUTH_UNAVAILABLE", "TLS_ERROR", "NETWORK_ERROR", "REF_CONFLICT",
            "REMOTE_REJECTED", "TIMEOUT",
        } else "UNKNOWN"


def git_failure(stderr: bytes) -> str:
    """Classify in memory; discard URLs, helper output and credential material."""
    message = stderr.lower()
    for category, markers in (
        ("AUTH_UNAVAILABLE", (b"could not read username", b"could not read password",
                              b"terminal prompts disabled", b"authentication failed")),
        ("TLS_ERROR", (b"ssl certificate", b"certificate verify failed")),
        ("NETWORK_ERROR", (b"connection reset", b"failed to connect", b"could not resolve",
                           b"recv failure", b"empty reply", b"timed out")),
        ("REF_CONFLICT", (b"non-fast-forward", b"fetch first", b"cannot lock ref")),
        ("REMOTE_REJECTED", (b"permission denied", b"remote rejected", b"error: 403")),
    ):
        if any(marker in message for marker in markers):
            return category
    return "UNKNOWN"


def now_local() -> datetime:
    return datetime.now(TZ)


def cycle_date(value: str) -> date:
    if not isinstance(value, str) or not DATE_RE.fullmatch(value):
        raise HealthError("INVALID_DATE")
    try:
        return date.fromisoformat(value)
    except ValueError:
        raise HealthError("INVALID_DATE") from None


def stamp(value: datetime) -> str:
    return value.astimezone(TZ).isoformat(timespec="seconds")


def timestamp(value: object) -> datetime:
    if not isinstance(value, str) or not re.fullmatch(r"\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\+08:00", value):
        raise HealthError("INVALID_RECEIPT")
    try:
        return datetime.fromisoformat(value)
    except ValueError:
        raise HealthError("INVALID_RECEIPT") from None


def encode(payload: dict) -> bytes:
    return (json.dumps(payload, ensure_ascii=True, sort_keys=True, separators=(",", ":")) + "\n").encode()


def validate_sync(value: object, attempt: dict) -> dict:
    """Accept fixed public categories only, never infer success from exit zero."""
    if not isinstance(value, dict) or set(value) != SYNC_KEYS:
        raise HealthError("INVALID_RECEIPT")
    if (not isinstance(value["sourceState"], str) or value["sourceState"] not in SOURCE_STATES
            or not isinstance(value["syncState"], str) or value["syncState"] not in SYNC_STATES
            or not isinstance(value["reason"], str) or value["reason"] not in SYNC_REASONS):
        raise HealthError("INVALID_RECEIPT")
    finished = timestamp(attempt["finishedAt"]) if attempt["finishedAt"] is not None else None
    successful = timestamp(value["lastSuccessfulSyncAt"]) if value["lastSuccessfulSyncAt"] is not None else None
    if finished is None or (successful is not None and successful > finished):
        raise HealthError("INVALID_RECEIPT")
    if value["syncState"] == "SUCCEEDED":
        entered = timestamp(attempt["enteredAt"]) if attempt["enteredAt"] is not None else None
        if (attempt["outcome"] != "RETURNED" or attempt["exitCode"] != 0 or entered is None
                or value["sourceState"] != "AVAILABLE" or value["reason"] != "NONE"
                or successful is None or successful < entered):
            raise HealthError("INVALID_RECEIPT")
    elif value["reason"] == "NONE":
        raise HealthError("INVALID_RECEIPT")
    return value


def validate(payload: object, cycle: str, now: datetime | None = None) -> dict:
    cycle_date(cycle)
    if not isinstance(payload, dict) or set(payload) != {"schemaVersion", "cycleDate", "attempts"}:
        raise HealthError("INVALID_RECEIPT")
    if type(payload["schemaVersion"]) is not int or payload["schemaVersion"] != 1 or payload["cycleDate"] != cycle:
        raise HealthError("INVALID_RECEIPT")
    attempts = payload["attempts"]
    if not isinstance(attempts, list) or not 1 <= len(attempts) <= 16 or len(encode(payload)) > MAX_BYTES:
        raise HealthError("INVALID_RECEIPT")
    previous = None
    for ordinal, attempt in enumerate(attempts, 1):
        if not isinstance(attempt, dict) or set(attempt) not in (ATTEMPT_KEYS, ATTEMPT_KEYS | {"sync"}):
            raise HealthError("INVALID_RECEIPT")
        if type(attempt["attemptNo"]) is not int or attempt["attemptNo"] != ordinal:
            raise HealthError("INVALID_RECEIPT")
        invoked = timestamp(attempt["invokedAt"])
        entered = timestamp(attempt["enteredAt"]) if attempt["enteredAt"] is not None else None
        finished = timestamp(attempt["finishedAt"]) if attempt["finishedAt"] is not None else None
        if invoked.date().isoformat() != cycle or (previous and invoked < previous):
            raise HealthError("INVALID_RECEIPT")
        if (entered and entered < invoked) or (finished and finished < (entered or invoked)):
            raise HealthError("INVALID_RECEIPT")
        if now and any(value > now + timedelta(seconds=2) for value in (invoked, entered, finished) if value):
            raise HealthError("INVALID_RECEIPT")
        previous = invoked
        outcome, code, reason = attempt["outcome"], attempt["exitCode"], attempt["skipReason"]
        if code is not None and (type(code) is not int or not -255 <= code <= 255):
            raise HealthError("INVALID_RECEIPT")
        if outcome == "RUNNING":
            valid = finished is None and code is None and reason is None
        elif outcome == "RETURNED":
            valid = finished is not None and type(code) is int and code >= 0 and reason is None
        elif outcome == "SKIPPED":
            valid = entered is None and finished is not None and code == 0 and reason in ("LOCKED", "MAINTENANCE")
        elif outcome == "PROCESS_ERROR":
            valid = finished is not None and (code is None or code < 0) and reason is None
        else:
            valid = False
        if not valid:
            raise HealthError("INVALID_RECEIPT")
        if "sync" in attempt:
            validate_sync(attempt["sync"], attempt)
    return payload


def decode(raw: bytes, cycle: str, now: datetime | None = None) -> dict:
    if len(raw) > MAX_BYTES:
        raise HealthError("INVALID_RECEIPT")
    def unique_pairs(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise HealthError("INVALID_RECEIPT")
            result[key] = value
        return result
    try:
        return validate(json.loads(raw, object_pairs_hook=unique_pairs), cycle, now)
    except (ValueError, TypeError, UnicodeError):
        raise HealthError("INVALID_RECEIPT") from None


def atomic_json(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.is_symlink():
        raise HealthError("LOCAL_STATE_ERROR")
    fd, name = tempfile.mkstemp(prefix=".health-", dir=path.parent)
    temporary = Path(name)
    try:
        with os.fdopen(fd, "wb") as handle:
            handle.write(encode(payload))
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


@contextmanager
def exclusive(path: Path, wait: float = 0):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a") as handle:
        deadline = time.monotonic() + wait
        while True:
            try:
                fcntl.flock(handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
                break
            except BlockingIOError:
                if time.monotonic() >= deadline:
                    raise
                time.sleep(min(0.1, max(0, deadline - time.monotonic())))
        yield


def health_dir(root: Path) -> Path:
    return root / ".yoj-sync" / "health"


def read_receipt(path: Path, now: datetime | None = None) -> dict:
    if path.is_symlink() or not path.is_file():
        raise HealthError("LOCAL_STATE_ERROR")
    with path.open("rb") as handle:
        return decode(handle.read(MAX_BYTES + 1), path.stem, now)


def network_window(now: datetime) -> bool:
    return day_time(22, 30) <= now.astimezone(TZ).time() < day_time(23, 55)


def clean_environment() -> dict[str, str]:
    # Git's existing OS credential helper remains usable. No YOJ environment
    # or arbitrary GIT_* configuration is inherited by the delivery process.
    allowed = ("PATH", "HOME", "TMPDIR", "LANG", "LC_ALL", "SSL_CERT_FILE", "SSL_CERT_DIR")
    environment = {key: os.environ[key] for key in allowed if key in os.environ}
    environment["GIT_TERMINAL_PROMPT"] = "0"
    return environment


def spawn_delivery(root: Path) -> None:
    if os.environ.get("YOJ_SYNC_ALLOW_PUSH") != "1" or not network_window(now_local()):
        return
    environment = clean_environment()
    environment["YOJ_SYNC_ALLOW_PUSH"] = "1"
    subprocess.Popen(
        [sys.executable, "-B", str(Path(__file__).resolve()), "flush", "--root", str(root)],
        env=environment, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL, start_new_session=True, close_fds=True,
    )


def observe(root: Path, event: str, *, token: str | None = None,
            returncode: int | None = None, reason: str | None = None,
            sync: dict | None = None) -> str | None:
    """Best-effort local hooks: return a context token, never raise into sync."""
    try:
        directory = health_dir(root)
        current = now_local()
        if event == "flush":
            spawn_delivery(root)
            return None
        with exclusive(directory / "journal.lock"):
            if event == "begin":
                if not day_time(22, 30) <= current.time() < day_time(23, 30):
                    return None
                cycle = current.date().isoformat()
                path = directory / "receipts" / f"{cycle}.json"
                payload = read_receipt(path, current) if path.exists() else {"schemaVersion": 1, "cycleDate": cycle, "attempts": []}
                attempt = {"attemptNo": len(payload["attempts"]) + 1, "invokedAt": stamp(current),
                           "enteredAt": None, "finishedAt": None, "outcome": "RUNNING",
                           "exitCode": None, "skipReason": None}
                payload["attempts"].append(attempt)
                validate(payload, cycle, current)
                token = uuid.uuid4().hex
                atomic_json(path, payload)
                atomic_json(directory / "contexts" / f"{token}.json", {"cycleDate": cycle, "attemptNo": attempt["attemptNo"]})
                return token
            if not token or not TOKEN_RE.fullmatch(token):
                return None
            context_path = directory / "contexts" / f"{token}.json"
            if context_path.is_symlink() or context_path.stat().st_size > 256:
                return None
            context = json.loads(context_path.read_bytes())
            if set(context) != {"cycleDate", "attemptNo"}:
                return None
            cycle_date(context["cycleDate"])
            path = directory / "receipts" / f"{context['cycleDate']}.json"
            payload = read_receipt(path, current)
            ordinal = context["attemptNo"]
            if type(ordinal) is not int or not 1 <= ordinal <= len(payload["attempts"]):
                return None
            attempt = payload["attempts"][ordinal - 1]
            if attempt["outcome"] != "RUNNING":
                return None
            if event == "entered" and attempt["enteredAt"] is None:
                attempt["enteredAt"] = stamp(current)
            elif event == "skipped":
                attempt.update(outcome="SKIPPED", finishedAt=stamp(current), exitCode=0, skipReason=reason)
            elif event == "finished":
                attempt.update(outcome="RETURNED" if returncode is not None and returncode >= 0 else "PROCESS_ERROR",
                               finishedAt=stamp(current), exitCode=returncode)
                if sync is not None:
                    # Invalid optional evidence must not lose the authentic
                    # terminal receipt or alter the scheduler's return code.
                    try:
                        attempt["sync"] = json.loads(encode(validate_sync(sync, attempt)))
                    except (HealthError, TypeError, ValueError, UnicodeError):
                        pass
            else:
                return None
            validate(payload, context["cycleDate"], current)
            atomic_json(path, payload)
        spawn_delivery(root)
    except Exception:
        # Telemetry failure must not change the business result, including
        # when the disk, process launcher, lock or Git credential is unavailable.
        return None
    return None


def merge_receipts(remote: dict, local: dict) -> dict:
    validate(remote, remote["cycleDate"])
    validate(local, remote["cycleDate"])
    merged = json.loads(encode(remote))
    for candidate in local["attempts"]:
        index = candidate["attemptNo"] - 1
        if index >= len(merged["attempts"]):
            merged["attempts"].append(candidate)
            continue
        existing = merged["attempts"][index]
        if existing["invokedAt"] != candidate["invokedAt"]:
            raise HealthError("RECEIPT_CONFLICT")
        if existing == candidate:
            continue
        if existing["outcome"] != "RUNNING":
            if candidate["outcome"] != "RUNNING":
                # A schema-1 writer may still replay the identical terminal
                # attempt without optional sync evidence. Preserve evidence,
                # and only accept a one-way addition to an unchanged attempt.
                base_existing = {key: existing[key] for key in ATTEMPT_KEYS}
                base_candidate = {key: candidate[key] for key in ATTEMPT_KEYS}
                if base_existing != base_candidate:
                    raise HealthError("RECEIPT_CONFLICT")
                if "sync" not in existing:
                    merged["attempts"][index] = candidate
                elif "sync" in candidate and existing["sync"] != candidate["sync"]:
                    raise HealthError("RECEIPT_CONFLICT")
            continue
        if existing["enteredAt"] and existing["enteredAt"] != candidate["enteredAt"]:
            if candidate["enteredAt"] is None and candidate["outcome"] == "RUNNING":
                continue
            raise HealthError("RECEIPT_CONFLICT")
        merged["attempts"][index] = candidate
    return validate(merged, remote["cycleDate"])


class GitDelivery:
    """An isolated object database with one fixed destination and no main ref."""

    def __init__(self, root: Path, budget: float = DELIVERY_BUDGET):
        self.directory = root / ".yoj-sync" / "health.git"
        self.deadline = time.monotonic() + budget
        self.environment = clean_environment()

    def git(self, *args: str, data: bytes | None = None) -> bytes:
        step = args[0] if args else "OTHER"
        remaining = self.deadline - time.monotonic()
        if remaining <= 0:
            raise GitCommandError("DELIVERY_TIMEOUT", step, "TIMEOUT")
        process = subprocess.Popen(
            ["git", "-c", "core.hooksPath=/dev/null", "-c", "commit.gpgSign=false",
             "-c", f"user.name={AUTHOR}", "-c", f"user.email={EMAIL}",
             "--git-dir", str(self.directory), *args],
            env=self.environment, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, start_new_session=True,
        )
        try:
            output, stderr = process.communicate(data, timeout=min(remaining, GIT_STEP_TIMEOUT))
        except subprocess.TimeoutExpired:
            # Kill this Git process group, including stalled transport/helpers;
            # the business scheduler belongs to a different process group.
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            process.communicate(timeout=1)
            raise GitCommandError("DELIVERY_TIMEOUT", step, "TIMEOUT") from None
        if process.returncode:
            raise GitCommandError("GIT_DELIVERY_FAILED", step, git_failure(stderr))
        return output

    def snapshot(self) -> tuple[str | None, dict[str, tuple[str, dict]]]:
        self.directory.parent.mkdir(parents=True, exist_ok=True)
        if self.directory.is_symlink():
            raise HealthError("LOCAL_STATE_ERROR")
        if not self.directory.exists():
            self.git("init", "--bare", str(self.directory))
        # A failed lookup must never be treated as an absent branch.
        refs = self.git("ls-remote", "--refs", REMOTE, REF).decode().splitlines()
        if not refs:
            return None, {}
        if len(refs) != 1 or refs[0].split()[1] != REF or not SHA_RE.fullmatch(refs[0].split()[0]):
            raise HealthError("INVALID_REMOTE")
        self.git("fetch", "--no-tags", "--depth=1", REMOTE, REF)
        parent = self.git("rev-parse", "FETCH_HEAD").decode().strip()
        rows = self.git("ls-tree", "-rz", parent).split(b"\0")
        entries = []
        for row in filter(None, rows):
            meta, name = row.split(b"\t", 1)
            mode, kind, sha = meta.decode().split()
            path = name.decode()
            match = PATH_RE.fullmatch(path)
            if mode != "100644" or kind != "blob" or not match or not SHA_RE.fullmatch(sha):
                raise HealthError("INVALID_REMOTE_TREE")
            entries.append((path, sha, match[1]))
        if len(entries) > 4000:
            raise HealthError("INVALID_REMOTE_TREE")
        records = {}
        if entries:
            ids = ("\n".join(entry[1] for entry in entries) + "\n").encode()
            sizes = self.git("cat-file", "--batch-check", data=ids).decode().splitlines()
            if len(sizes) != len(entries):
                raise HealthError("INVALID_REMOTE_TREE")
            for line, (_, sha, _) in zip(sizes, entries):
                fields = line.split()
                if len(fields) != 3 or fields[:2] != [sha, "blob"] or not 0 <= int(fields[2]) <= MAX_BYTES:
                    raise HealthError("INVALID_REMOTE_TREE")
            raw = self.git("cat-file", "--batch", data=ids)
            position = 0
            for path, sha, cycle in entries:
                end = raw.index(b"\n", position)
                fields = raw[position:end].decode().split()
                size = int(fields[2])
                if fields[:2] != [sha, "blob"] or size > MAX_BYTES:
                    raise HealthError("INVALID_REMOTE_TREE")
                start = end + 1
                records[path] = (sha, decode(raw[start:start + size], cycle, now_local()))
                position = start + size + 1
        return parent, records

    def publish(self, receipts: list[dict], initialize: bool = False) -> str | None:
        parent, records = self.snapshot()
        changed = parent is None and initialize
        for receipt in receipts:
            cycle = receipt["cycleDate"]
            validate(receipt, cycle, now_local())
            path = f"receipts/{cycle}.json"
            prior = records.get(path)
            merged = merge_receipts(prior[1], receipt) if prior else receipt
            if prior and merged == prior[1]:
                continue
            sha = self.git("hash-object", "-w", "--stdin", data=encode(merged)).decode().strip()
            records[path] = (sha, merged)
            changed = True
        if not changed:
            return parent
        if records:
            listing = "".join(f"100644 blob {sha}\t{Path(path).name}\n" for path, (sha, _) in sorted(records.items()))
            subtree = self.git("mktree", data=listing.encode()).decode().strip()
            tree = self.git("mktree", data=f"040000 tree {subtree}\treceipts\n".encode()).decode().strip()
        else:
            tree = self.git("mktree", data=b"").decode().strip()
        commit = self.git("commit-tree", tree, *(["-p", parent] if parent else []),
                          data=b"chore: record nightly scheduler receipts\n").decode().strip()
        # Only the isolated, schema-checked tree can be pushed. Never force.
        self.git("push", "--no-follow-tags", REMOTE, f"{commit}:{REF}")
        return commit


def deliver(root: Path, initialize: bool = False, replay: bool = False) -> str | None:
    """Send authentic journals only; replay is an explicit operator recovery.

    Automatic delivery stays inside the nightly/buffer window. Replay changes
    neither timestamps nor outcomes and cannot start the business scheduler.
    """
    current = now_local()
    if os.environ.get("YOJ_SYNC_ALLOW_PUSH") != "1":
        raise HealthError("PUSH_DISABLED")
    if initialize and replay:
        raise HealthError("INVALID_DELIVERY_MODE")
    if initialize or replay:
        if current.time() >= day_time(23, 55) or current.time() < day_time(0, 10):
            raise HealthError("OUTSIDE_DELIVERY_WINDOW")
    elif not network_window(current):
        return None
    directory = health_dir(root)
    started = time.monotonic()
    with exclusive(directory / "delivery.lock", wait=20):
        current = now_local()
        if current.time() >= day_time(23, 55) or current.time() < day_time(0, 10):
            return None
        if not (initialize or replay) and not network_window(current):
            return None
        # Initialization publishes an empty branch, never a fabricated run.
        seconds_left = (datetime.combine(current.date(), day_time(23, 55), TZ) - current).total_seconds()
        publisher = GitDelivery(root, min(DELIVERY_BUDGET - (time.monotonic() - started), seconds_left))
        failures = 0
        try:
            while True:
                receipts = [] if initialize else [read_receipt(path, now_local()) for path in sorted((directory / "receipts").glob("*.json"))]
                try:
                    result = publisher.publish(receipts, initialize)
                except HealthError as exc:
                    failures += 1
                    if (str(exc) not in {"DELIVERY_TIMEOUT", "GIT_DELIVERY_FAILED"}
                            or failures >= 3 or publisher.deadline - time.monotonic() <= 2):
                        raise
                    time.sleep(1)
                    # Refresh the branch on retry, including after an ambiguous
                    # push result; never blindly repeat a stale/force push.
                    continue
                # Coalesce a terminal journal update arriving during upload.
                latest = [] if initialize else [read_receipt(path, now_local()) for path in sorted((directory / "receipts").glob("*.json"))]
                if latest == receipts:
                    break
            atomic_json(directory / "delivery-status.json", {"state": "DELIVERED", "checkedAt": stamp(now_local())})
            return result
        except Exception as exc:
            # Persist fixed categories locally: no Git stderr or credentials.
            category = str(exc) if isinstance(exc, HealthError) and str(exc) in DELIVERY_ERRORS else "LOCAL_STATE_ERROR"
            status = {
                "state": "DELIVERY_FAILED", "checkedAt": stamp(now_local()), "category": category,
            }
            if isinstance(exc, GitCommandError):
                status.update(gitStep=exc.step, gitFailure=exc.failure)
            atomic_json(directory / "delivery-status.json", status)
            raise


def expected_cycles(now: datetime, enabled_from: str, requested: str = "", lookback: int = 2) -> list[str]:
    enabled = cycle_date(enabled_from)
    local = now.astimezone(TZ)
    latest = local.date() - timedelta(days=int(local.time() < day_time(23, 55)))
    if requested:
        selected = cycle_date(requested)
        if selected > latest:
            raise HealthError("CYCLE_NOT_DUE")
        return [selected.isoformat()] if selected >= enabled else []
    if not 1 <= lookback <= 7:
        raise HealthError("INVALID_LOOKBACK")
    return [(latest - timedelta(days=offset)).isoformat() for offset in range(lookback)
            if latest - timedelta(days=offset) >= enabled]


def classify(payload: dict, cycle: str, now: datetime) -> dict:
    validate(payload, cycle, now)
    start = datetime.combine(cycle_date(cycle), day_time(22, 30), TZ)
    end = datetime.combine(cycle_date(cycle), day_time(23, 30), TZ)
    attempts = payload["attempts"]
    entered = [a for a in attempts if a["enteredAt"] is not None]
    timely = [a for a in entered if start <= timestamp(a["invokedAt"]) < end and start <= timestamp(a["enteredAt"]) < end]
    schedule = ("ON_TIME" if timely else "OUT_OF_WINDOW" if entered else
                "NOT_ENTERED" if all(a["outcome"] == "SKIPPED" for a in attempts) else "ENTRY_UNCONFIRMED")
    latest = attempts[-1]
    if latest["outcome"] == "RUNNING":
        result = "NO_TERMINAL_RECEIPT"
    elif latest["outcome"] == "RETURNED":
        result = "RETURNED_OK" if latest["exitCode"] == 0 else "RETURNED_NONZERO"
    else:
        result = latest["outcome"]
    evidence = latest.get("sync", {})
    successful = [a["sync"]["lastSuccessfulSyncAt"] for a in attempts
                  if a.get("sync", {}).get("lastSuccessfulSyncAt") is not None]
    return {"cycleDate": cycle, "scheduleState": schedule, "runState": result,
            "sourceState": evidence.get("sourceState", "UNKNOWN"),
            "syncState": evidence.get("syncState", "UNKNOWN"),
            "reason": evidence.get("reason", "UNKNOWN"),
            "lastSuccessfulSyncAt": max(successful) if successful else None,
            "attempts": attempts}


def public_json(endpoint: str) -> dict | None:
    """Anonymous, bounded API access. None means a genuine HTTP 404."""
    request = urllib.request.Request(f"https://api.github.com/repos/{REPOSITORY}/{endpoint}",
                                     headers={"Accept": "application/vnd.github+json", "User-Agent": "YOJ-nightly-health", "Cache-Control": "no-cache"})
    use_system_tls = False
    for attempt in range(3):
        try:
            if use_system_tls:
                # Some macOS Python installations lack the OS trust roots.
                # Retain certificate verification and the same anonymous URL;
                # --disable prevents ~/.curlrc from adding credentials/options.
                response = subprocess.run(
                    ["curl", "--disable", "--silent", "--show-error", "--location",
                     "--proto", "=https", "--proto-redir", "=https", "--max-redirs", "3",
                     "--max-time", "10", "--max-filesize", "32768", "--write-out", "\n%{http_code}",
                     "-H", "Accept: application/vnd.github+json", "-H", "User-Agent: YOJ-nightly-health",
                     "-H", "Cache-Control: no-cache", request.full_url],
                    env=clean_environment(), capture_output=True, timeout=12, check=False,
                )
                raw, _, status = response.stdout.rpartition(b"\n")
                if response.returncode:
                    raise OSError("SYSTEM_HTTPS_UNAVAILABLE")
                if status == b"404":
                    return None
                if status != b"200":
                    raise OSError("SYSTEM_HTTPS_UNAVAILABLE")
            else:
                with urllib.request.urlopen(request, timeout=10) as response:
                    raw = response.read(32769)
            if len(raw) > 32768:
                raise HealthError("OBSERVATION_ERROR")
            value = json.loads(raw)
            if not isinstance(value, dict):
                raise HealthError("OBSERVATION_ERROR")
            return value
        except urllib.error.HTTPError as exc:
            # HTTPError also owns a response stream; release it on every
            # retry and on 404 without changing the observation semantics.
            try:
                exc.close()
            except OSError:
                pass
            if exc.code == 404:
                return None
        except (OSError, subprocess.SubprocessError):
            use_system_tls = True
        except (ValueError, UnicodeError):
            pass
        if attempt < 2:
            time.sleep(attempt + 1)
    raise HealthError("OBSERVATION_ERROR")


def check_cycles(cycles: list[str], now: datetime) -> list[dict]:
    ref = public_json(f"git/ref/heads/{BRANCH}")
    if ref is None:
        # A missing repository must not be confused with a missing receipt.
        if public_json("") is None:
            raise HealthError("OBSERVATION_ERROR")
        return [{"cycleDate": cycle, "scheduleState": "NO_RECEIPT", "runState": "UNKNOWN"} for cycle in cycles]
    sha = ref.get("object", {}).get("sha", "")
    if ref.get("ref") != REF or not isinstance(sha, str) or not SHA_RE.fullmatch(sha):
        raise HealthError("OBSERVATION_ERROR")
    results = []
    for cycle in cycles:
        item = public_json(f"contents/receipts/{cycle}.json?ref={sha}")
        if item is None:
            results.append({"cycleDate": cycle, "scheduleState": "NO_RECEIPT", "runState": "UNKNOWN"})
            continue
        try:
            if item.get("type") != "file" or item.get("encoding") != "base64" or type(item.get("size")) is not int or not 0 <= item["size"] <= MAX_BYTES:
                raise HealthError("INVALID_RECEIPT")
            raw = base64.b64decode("".join(item["content"].split()), validate=True)
            results.append(classify(decode(raw, cycle, now), cycle, now))
        except (HealthError, ValueError, KeyError, TypeError):
            results.append({"cycleDate": cycle, "scheduleState": "INVALID_RECEIPT", "runState": "UNKNOWN"})
    return results


def render_summary(results: list[dict], error: str | None = None) -> str:
    lines = ["# Local nightly schedule and sync receipts", "",
             "Scheduling, source access and sync completion are separate evidence. The default exit status checks scheduling only.", "",
             "Legacy receipts report sync UNKNOWN even with exit 0. Receipt replay preserves original outcomes and success times; it does not prove a new sync or Pages deployment.", ""]
    if error:
        lines.append(f"Check unavailable: `{error}`.")
    elif not results:
        lines.append("NOT_DUE: no monitored nightly cycle has reached its deadline.")
    else:
        lines.extend(["| Cycle (Asia/Shanghai) | Scheduling | Execution | Source | Sync | Reason | Last successful sync (Asia/Shanghai) |",
                      "| --- | --- | --- | --- | --- | --- | --- |"])
        for result in results:
            lines.append(f"| {result['cycleDate']} | {result['scheduleState']} | {result['runState']} | {result.get('sourceState', 'UNKNOWN')} | {result.get('syncState', 'UNKNOWN')} | {result.get('reason', 'UNKNOWN')} | {result.get('lastSuccessfulSyncAt') or 'unknown'} |")
            for attempt in result.get("attempts", []):
                sync = attempt.get("sync", {})
                lines.append(f"| Attempt {attempt['attemptNo']} | entered: {attempt['enteredAt'] or 'unconfirmed'} | {attempt['outcome']}, exit: {attempt['exitCode']} | {sync.get('sourceState', 'UNKNOWN')} | {sync.get('syncState', 'UNKNOWN')} | {sync.get('reason', 'UNKNOWN')} | {sync.get('lastSuccessfulSyncAt') or 'unknown'} |")
        lines.extend(["", "NO_RECEIPT means no remote evidence; it does not identify a local or network failure."])
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    for name in ("flush", "initialize", "replay"):
        child = commands.add_parser(name)
        child.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    check = commands.add_parser("check")
    check.add_argument("--enabled-from", default=ENABLED_FROM)
    check.add_argument("--cycle-date", default="")
    check.add_argument("--lookback-days", type=int, default=2)
    check.add_argument("--summary", type=Path)
    check.add_argument("--require-sync", action="store_true",
                       help="Also fail when a completed cycle lacks explicit successful sync evidence")
    args = parser.parse_args()
    if args.command != "check":
        try:
            result = deliver(args.root.resolve(), initialize=args.command == "initialize", replay=args.command == "replay")
            print(result or "NO_DELIVERY")
            return 0
        except Exception:
            # No raw Git stderr, credentials, local paths or exception text.
            print("HEALTH_DELIVERY_UNAVAILABLE")
            return 1
    results, error = [], None
    try:
        current = now_local()
        cycles = expected_cycles(current, args.enabled_from, args.cycle_date, args.lookback_days)
        results = check_cycles(cycles, current)
    except HealthError as exc:
        error = str(exc)
    except Exception:
        error = "OBSERVATION_ERROR"
    summary = render_summary(results, error)
    print(summary)
    if args.summary:
        args.summary.write_text(summary, encoding="utf-8")
    if error:
        print(f"::error::Receipt check unavailable: {error}; scheduling is unknown")
    for result in results:
        if result["scheduleState"] != "ON_TIME":
            print(f"::error::{result['cycleDate']}: scheduling={result['scheduleState']}; no confirmed on-time receipt")
        elif result["runState"] != "RETURNED_OK":
            print(f"::warning::{result['cycleDate']}: scheduled on time; execution={result['runState']}")
        if result.get("syncState", "UNKNOWN") != "SUCCEEDED":
            level = "error" if args.require_sync else "warning"
            print(f"::{level}::{result['cycleDate']}: source={result.get('sourceState', 'UNKNOWN')}; sync={result.get('syncState', 'UNKNOWN')}; reason={result.get('reason', 'UNKNOWN')}; last successful sync={result.get('lastSuccessfulSyncAt') or 'unknown'}")
    return int(bool(error) or any(result["scheduleState"] != "ON_TIME" for result in results)
               or (args.require_sync and any(result.get("syncState", "UNKNOWN") != "SUCCEEDED" for result in results)))


if __name__ == "__main__":
    raise SystemExit(main())
