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
ATTEMPT_KEYS = {"attemptNo", "invokedAt", "enteredAt", "finishedAt", "outcome", "exitCode", "skipReason"}
DATE_RE = re.compile(r"\d{4}-\d{2}-\d{2}\Z")
TOKEN_RE = re.compile(r"[0-9a-f]{32}\Z")
SHA_RE = re.compile(r"[0-9a-f]{40}\Z")
PATH_RE = re.compile(r"receipts/(\d{4}-\d{2}-\d{2})\.json\Z")


class HealthError(Exception):
    """Only fixed error categories may reach public output."""


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
        if not isinstance(attempt, dict) or set(attempt) != ATTEMPT_KEYS:
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
            returncode: int | None = None, reason: str | None = None) -> str | None:
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

    def __init__(self, root: Path, budget: float = 20):
        self.directory = root / ".yoj-sync" / "health.git"
        self.deadline = time.monotonic() + budget
        self.environment = clean_environment()

    def git(self, *args: str, data: bytes | None = None) -> bytes:
        remaining = self.deadline - time.monotonic()
        if remaining <= 0:
            raise HealthError("DELIVERY_TIMEOUT")
        process = subprocess.Popen(
            ["git", "-c", "core.hooksPath=/dev/null", "-c", "commit.gpgSign=false",
             "-c", f"user.name={AUTHOR}", "-c", f"user.email={EMAIL}",
             "--git-dir", str(self.directory), *args],
            env=self.environment, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, start_new_session=True,
        )
        try:
            output, _ = process.communicate(data, timeout=remaining)
        except subprocess.TimeoutExpired:
            # Kill this Git process group, including stalled transport/helpers;
            # the business scheduler belongs to a different process group.
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            process.communicate(timeout=1)
            raise HealthError("DELIVERY_TIMEOUT") from None
        if process.returncode:
            raise HealthError("GIT_DELIVERY_FAILED")
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


def deliver(root: Path, initialize: bool = False) -> str | None:
    current = now_local()
    if os.environ.get("YOJ_SYNC_ALLOW_PUSH") != "1":
        raise HealthError("PUSH_DISABLED")
    if initialize:
        if current.time() >= day_time(23, 55) or current.time() < day_time(0, 10):
            raise HealthError("OUTSIDE_DELIVERY_WINDOW")
    elif not network_window(current):
        return None
    directory = health_dir(root)
    started = time.monotonic()
    with exclusive(directory / "delivery.lock", wait=20):
        current = now_local()
        if not initialize and not network_window(current):
            return None
        # Initialization publishes an empty branch, never a fabricated run.
        seconds_left = (datetime.combine(current.date(), day_time(23, 55), TZ) - current).total_seconds()
        publisher = GitDelivery(root, min(20 - (time.monotonic() - started), seconds_left))
        while True:
            receipts = [] if initialize else [read_receipt(path, now_local()) for path in sorted((directory / "receipts").glob("*.json"))]
            result = publisher.publish(receipts, initialize)
            # A fast no-op can finish while its RUNNING receipt is uploading.
            # Coalesce that final journal update within the same worker budget.
            latest = [] if initialize else [read_receipt(path, now_local()) for path in sorted((directory / "receipts").glob("*.json"))]
            if latest == receipts:
                break
        atomic_json(directory / "delivery-status.json", {"state": "DELIVERED", "checkedAt": stamp(now_local())})
        return result


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
    return {"cycleDate": cycle, "scheduleState": schedule, "runState": result, "attempts": attempts}


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
    lines = ["# Local nightly schedule receipts", "", "This check confirms scheduling, not YOJ acceptance or Pages deployment.", ""]
    if error:
        lines.append(f"Check unavailable: `{error}`.")
    elif not results:
        lines.append("NOT_DUE: no monitored nightly cycle has reached its deadline.")
    else:
        lines.extend(["| Cycle (Asia/Shanghai) | Scheduling | Execution |", "| --- | --- | --- |"])
        for result in results:
            lines.append(f"| {result['cycleDate']} | {result['scheduleState']} | {result['runState']} |")
            for attempt in result.get("attempts", []):
                lines.append(f"| Attempt {attempt['attemptNo']} | entered: {attempt['enteredAt'] or 'unconfirmed'} | {attempt['outcome']}, exit: {attempt['exitCode']} |")
        lines.extend(["", "NO_RECEIPT means no remote evidence; it does not identify a local or network failure."])
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    for name in ("flush", "initialize"):
        child = commands.add_parser(name)
        child.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    check = commands.add_parser("check")
    check.add_argument("--enabled-from", default=ENABLED_FROM)
    check.add_argument("--cycle-date", default="")
    check.add_argument("--lookback-days", type=int, default=2)
    check.add_argument("--summary", type=Path)
    args = parser.parse_args()
    if args.command != "check":
        try:
            result = deliver(args.root.resolve(), initialize=args.command == "initialize")
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
    for result in results:
        if result["scheduleState"] == "ON_TIME" and result["runState"] != "RETURNED_OK":
            print(f"::warning::{result['cycleDate']}: scheduled on time; execution={result['runState']}")
    return int(bool(error) or any(result["scheduleState"] != "ON_TIME" for result in results))


if __name__ == "__main__":
    raise SystemExit(main())
