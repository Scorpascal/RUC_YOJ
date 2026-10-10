#!/usr/bin/env python3
"""Gate the daily YOJ scheduler for a bounded launchd update window.

The LaunchAgent may load before the daily window, when the user logs in during
the window, or while the login keychain is still locked.  This guard allows
the full sync from 22:30 onward and, after that sync succeeds, performs a
lightweight public-visibility check every 15 minutes until 23:30 Beijing
time.  The 23:30–23:55 period is reserved as a flexibility buffer before the
maintenance window; it starts no new scheduler run. After waking outside the
business window, only undelivered authentic receipts may be retried. A missed day is skipped,
while scheduler checkpoints remain available for the next day's window.
"""

from __future__ import annotations

import fcntl
import json
import os
import stat
import subprocess
import sys
import uuid
from datetime import datetime, time as day_time
from pathlib import Path
from zoneinfo import ZoneInfo


ROOT = Path(os.environ.get("YOJ_ROOT", Path(__file__).resolve().parents[1])).resolve()
STATE_DIR = ROOT / ".yoj-sync"
GUARD_LOCK_PATH = STATE_DIR / "launchd-guard.lock"
GUARD_STATE_PATH = STATE_DIR / "launchd-cycle.json"
LOG_PATH = STATE_DIR / "launchd-guard.log"
SYNC_STATUS_NAME = "sync-status.json"
SYNC_STATUS_KEYS = {"schemaVersion", "startedAt", "finishedAt", "mode", "sourceState", "syncState",
                    "reason", "lastSuccessfulSyncAt", "retryAfter"}
MAX_STATUS_BYTES = 8192
INVOCATION_ENV = "YOJ_SYNC_INVOCATION_ID"
_last_run_sync: dict | None = None
_last_run_admitted = False
_last_run_skipped = False
_last_run_has_status = False
TZ = ZoneInfo("Asia/Shanghai")
UPDATE_WINDOW_START = day_time(22, 30)
UPDATE_WINDOW_END = day_time(23, 30)
MAINTENANCE_START = day_time(23, 55)
MAINTENANCE_END = day_time(0, 10)


def now_local() -> datetime:
    return datetime.now(TZ)


def log(message: str) -> None:
    STATE_DIR.mkdir(parents=True, exist_ok=True)
    line = f"{now_local().isoformat(timespec='seconds')} {message}\n"
    with LOG_PATH.open("a", encoding="utf-8") as handle:
        handle.write(line)
    print(message, flush=True)


def in_maintenance(value: datetime) -> bool:
    current = value.time()
    return current >= MAINTENANCE_START or current < MAINTENANCE_END


def in_update_window(value: datetime) -> bool:
    current = value.time()
    return UPDATE_WINDOW_START <= current < UPDATE_WINDOW_END


def cycle_key(value: datetime) -> str:
    """Return the calendar day for a run inside the update window."""

    return value.date().isoformat()


def load_state() -> dict[str, str]:
    if not GUARD_STATE_PATH.is_file():
        return {}
    try:
        payload = json.loads(GUARD_STATE_PATH.read_text(encoding="utf-8"))
    except (OSError, TypeError, ValueError, json.JSONDecodeError):
        return {}
    return {str(key): str(value) for key, value in payload.items() if value is not None}


def save_state(state: dict[str, str]) -> None:
    STATE_DIR.mkdir(parents=True, exist_ok=True)
    temporary = GUARD_STATE_PATH.with_suffix(".tmp")
    temporary.write_text(
        json.dumps({"schemaVersion": 1, **state}, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    os.replace(temporary, GUARD_STATE_PATH)


def scheduler_command(visibility_watch: bool = False) -> list[str]:
    if visibility_watch:
        return [
            sys.executable,
            str(ROOT / "tools" / "yoj_scheduler.py"),
            "--visibility-watch",
            "--publish",
            "--push",
        ]
    return [
        sys.executable,
        str(ROOT / "tools" / "yoj_scheduler.py"),
        "--allow-submit",
        "--publish",
        "--push",
    ]


def read_bounded_object(path: Path, limit: int) -> tuple[dict, os.stat_result]:
    """Read regular local state without following links or echoing raw data."""
    if path.is_symlink():
        raise ValueError("INVALID_LOCAL_STATE")
    if ROOT in path.parents:
        parent = path.parent
        while parent != ROOT:
            if parent.is_symlink():
                raise ValueError("INVALID_LOCAL_STATE")
            parent = parent.parent
    descriptor = os.open(path, os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0)
                         | getattr(os, "O_NONBLOCK", 0) | getattr(os, "O_CLOEXEC", 0))
    with os.fdopen(descriptor, "rb") as handle:
        metadata = os.fstat(handle.fileno())
        if not stat.S_ISREG(metadata.st_mode) or metadata.st_size > limit:
            raise ValueError("INVALID_LOCAL_STATE")
        raw = handle.read(limit + 1)
    if len(raw) > limit:
        raise ValueError("INVALID_LOCAL_STATE")
    def unique_pairs(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError("INVALID_LOCAL_STATE")
            result[key] = value
        return result
    value = json.loads(raw, object_pairs_hook=unique_pairs)
    if not isinstance(value, dict):
        raise ValueError("INVALID_LOCAL_STATE")
    return value, metadata


def observed_entry(context: str | None) -> str:
    """Distinguish confirmed skips from missing optional entry evidence."""
    if not context:
        return "UNKNOWN"
    try:
        if __package__:
            from . import yoj_health as health
        else:
            import yoj_health as health
        if not health.TOKEN_RE.fullmatch(context):
            return "UNKNOWN"
        directory = health.health_dir(ROOT)
        payload, _ = read_bounded_object(directory / "contexts" / f"{context}.json", 256)
        if set(payload) != {"cycleDate", "attemptNo"} or type(payload["attemptNo"]) is not int:
            return "UNKNOWN"
        health.cycle_date(payload["cycleDate"])
        receipt = health.read_receipt(directory / "receipts" / f"{payload['cycleDate']}.json", now_local())
        ordinal = payload["attemptNo"]
        if not 1 <= ordinal <= len(receipt["attempts"]):
            return "UNKNOWN"
        attempt = receipt["attempts"][ordinal - 1]
        if attempt["outcome"] == "SKIPPED":
            return "SKIPPED"
        if attempt["enteredAt"] is not None and attempt["outcome"] == "RUNNING":
            return "ENTERED"
        return "UNKNOWN"
    except Exception:
        return "UNKNOWN"


def read_current_sync(started: datetime, finished: datetime, returncode: int | None,
                      invocation_id: str | None = None) -> dict | None:
    """Optional status is accepted only for this completed full scheduler run."""
    try:
        if __package__:
            from . import yoj_health as health
        else:
            import yoj_health as health
        payload, metadata = read_bounded_object(ROOT / ".yoj-sync" / SYNC_STATUS_NAME, MAX_STATUS_BYTES)
        if (set(payload) not in (SYNC_STATUS_KEYS, SYNC_STATUS_KEYS | {"invocationId"})
                or type(payload["schemaVersion"]) is not int
                or payload["schemaVersion"] != 1 or payload["mode"] != "MAIN"
                or type(returncode) is not int):
            return None
        actual_id = payload.get("invocationId")
        if "invocationId" in payload and (not isinstance(actual_id, str) or not health.TOKEN_RE.fullmatch(actual_id)):
            return None
        if invocation_id is not None and (not isinstance(invocation_id, str)
                                         or not health.TOKEN_RE.fullmatch(invocation_id) or actual_id != invocation_id):
            return None
        status_started = health.timestamp(payload["startedAt"])
        status_finished = health.timestamp(payload["finishedAt"])
        if (not health.timestamp(health.stamp(started)) <= status_started <= status_finished
                <= health.timestamp(health.stamp(finished))
                or metadata.st_mtime < started.timestamp()
                or metadata.st_mtime > finished.timestamp() + 2):
            return None
        retry = health.timestamp(payload["retryAfter"]) if payload["retryAfter"] is not None else None
        if retry is not None and retry < status_finished:
            return None
        sync = {key: payload[key] for key in health.SYNC_KEYS}
        attempt = {"enteredAt": payload["startedAt"], "finishedAt": payload["finishedAt"],
                   "outcome": "RETURNED" if returncode >= 0 else "PROCESS_ERROR", "exitCode": returncode}
        health.validate_sync(sync, attempt)
        if sync["syncState"] == "SUCCEEDED" and sync["lastSuccessfulSyncAt"] != payload["finishedAt"]:
            return None
        return sync
    except Exception:
        # Stale/malformed state or telemetry I/O cannot change business exit.
        return None


def run_scheduler(visibility_watch: bool = False) -> int:
    global _last_run_sync, _last_run_admitted, _last_run_skipped, _last_run_has_status
    _last_run_sync, _last_run_admitted, _last_run_skipped, _last_run_has_status = None, False, False, False
    started = now_local()
    environment = os.environ.copy()
    environment["YOJ_ROOT"] = str(ROOT)
    invocation_id = uuid.uuid4().hex
    environment[INVOCATION_ENV] = invocation_id
    environment.pop("YOJ_HEALTH_CONTEXT", None)
    context = health_observe("begin") if not visibility_watch else None
    if context:
        environment["YOJ_HEALTH_CONTEXT"] = context
    result = None
    try:
        result = subprocess.run(
            scheduler_command(visibility_watch),
            cwd=ROOT,
            env=environment,
            check=False,
        )
    finally:
        finished = now_local()
        if not visibility_watch:
            entry = observed_entry(context)
            _last_run_admitted, _last_run_skipped = entry == "ENTERED", entry == "SKIPPED"
            path = ROOT / ".yoj-sync" / SYNC_STATUS_NAME
            try:
                _last_run_has_status = path.exists() or path.is_symlink()
            except OSError:
                _last_run_has_status = True
            # A concurrent admitted process may complete in the same second.
            # A known skip is stronger evidence than matching shared times.
            if not _last_run_skipped:
                _last_run_sync = read_current_sync(started, finished, result.returncode if result else None,
                                                  invocation_id=invocation_id)
        if context:
            health_observe("finished", token=context, returncode=result.returncode if result else None,
                           sync=_last_run_sync)
        else:
            health_observe("flush")
    log(f"调度器结束：exit={result.returncode}")
    return result.returncode


def health_observe(event: str, **kwargs: object) -> str | None:
    """Optional telemetry must not affect the existing guard control flow."""
    try:
        if __package__:
            from .yoj_health import observe
        else:
            from yoj_health import observe
        return observe(ROOT, event, **kwargs)
    except Exception:
        return None


def main() -> int:
    global _last_run_sync, _last_run_admitted, _last_run_skipped, _last_run_has_status
    current = now_local()
    if in_maintenance(current):
        log("处于北京时间 23:55–00:10 维护窗口，本次跳过网络操作")
        return 0
    if not in_update_window(current):
        if day_time(23, 30) <= current.time() < day_time(23, 55):
            # Retry only already-recorded telemetry. Never invoke the business
            # scheduler or manufacture a new cycle in the flexibility buffer.
            health_observe("flush")
            log("北京时间 23:30–23:55 为维护前弹性缓冲，不启动新一轮自动化")
        else:
            # Wake recovery is telemetry-only; the observer checks journal
            # acknowledgements and a 15-minute failure cooldown before spawn.
            health_observe("retry-pending")
            log("不在北京时间 22:30–23:30 更新窗口，本次跳过；错过当天不补跑")
        return 0

    STATE_DIR.mkdir(parents=True, exist_ok=True)
    cycle = cycle_key(current)
    with GUARD_LOCK_PATH.open("w", encoding="utf-8") as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            log("已有补偿守门器运行，本次退出")
            return 0

        # Re-read state after acquiring the lock so simultaneous launchd
        # calendar/interval events cannot start a duplicate cycle.
        state = load_state()
        if state.get("lastSuccessfulCycle") == cycle:
            log(f"主同步本周期已完成，执行轻量公开状态复查：cycle={cycle}")
            visibility_watch = True
        else:
            visibility_watch = False
        _last_run_sync, _last_run_admitted, _last_run_skipped, _last_run_has_status = None, False, False, False
        result = run_scheduler(visibility_watch=visibility_watch)
        main_succeeded = (result == 0 and not _last_run_skipped and (
            (_last_run_sync is not None and _last_run_sync["syncState"] == "SUCCEEDED")
            or (not _last_run_has_status and _last_run_admitted)))
        if result == 0 and (visibility_watch or main_succeeded):
            if visibility_watch:
                state["lastVisibilityWatchAt"] = now_local().isoformat(timespec="seconds")
            else:
                state["lastSuccessfulCycle"] = cycle
                state["lastSuccessfulAt"] = (_last_run_sync["lastSuccessfulSyncAt"] if _last_run_sync
                                             else now_local().isoformat(timespec="seconds"))
            save_state(state)
            log(f"记录{'轻量复查' if visibility_watch else '主同步'}成功：cycle={cycle}")
        else:
            detail = (f"；sync={_last_run_sync['syncState']}；reason={_last_run_sync['reason']}"
                      if _last_run_sync else "；主同步成功证据不足" if result == 0 else "")
            log(f"{'轻量复查' if visibility_watch else '本周期同步'}未完成，保留断点等待重试：cycle={cycle}{detail}")
        return result


if __name__ == "__main__":
    raise SystemExit(main())
