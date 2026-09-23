#!/usr/bin/env python3
"""Gate the daily YOJ scheduler for a bounded launchd update window.

The LaunchAgent may load before the daily window, when the user logs in during
the window, or while the login keychain is still locked.  This guard allows
the full sync from 22:30 onward and, after that sync succeeds, performs a
lightweight public-visibility check every 15 minutes until 23:30 Beijing
time.  The 23:30–23:55 period is reserved as a flexibility buffer before the
maintenance window; it starts no new scheduler run. A missed day is skipped,
while scheduler checkpoints remain available for the next day's window.
"""

from __future__ import annotations

import fcntl
import json
import os
import subprocess
import sys
from datetime import datetime, time as day_time
from pathlib import Path
from zoneinfo import ZoneInfo


ROOT = Path(os.environ.get("YOJ_ROOT", Path(__file__).resolve().parents[1])).resolve()
STATE_DIR = ROOT / ".yoj-sync"
GUARD_LOCK_PATH = STATE_DIR / "launchd-guard.lock"
GUARD_STATE_PATH = STATE_DIR / "launchd-cycle.json"
LOG_PATH = STATE_DIR / "launchd-guard.log"
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


def run_scheduler(visibility_watch: bool = False) -> int:
    environment = os.environ.copy()
    environment["YOJ_ROOT"] = str(ROOT)
    result = subprocess.run(
        scheduler_command(visibility_watch),
        cwd=ROOT,
        env=environment,
        check=False,
    )
    log(f"调度器结束：exit={result.returncode}")
    return result.returncode


def main() -> int:
    current = now_local()
    if in_maintenance(current):
        log("处于北京时间 23:55–00:10 维护窗口，本次跳过网络操作")
        return 0
    if not in_update_window(current):
        if day_time(23, 30) <= current.time() < day_time(23, 55):
            log("北京时间 23:30–23:55 为维护前弹性缓冲，不启动新一轮自动化")
        else:
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
        result = run_scheduler(visibility_watch=visibility_watch)
        if result == 0:
            if visibility_watch:
                state["lastVisibilityWatchAt"] = now_local().isoformat(timespec="seconds")
            else:
                state["lastSuccessfulCycle"] = cycle
                state["lastSuccessfulAt"] = now_local().isoformat(timespec="seconds")
            save_state(state)
            log(f"记录{'轻量复查' if visibility_watch else '主同步'}成功：cycle={cycle}")
        else:
            log(f"{'轻量复查' if visibility_watch else '本周期同步'}未完成，保留断点等待重试：cycle={cycle}")
        return result


if __name__ == "__main__":
    raise SystemExit(main())
