#!/usr/bin/env python3
"""Gate the daily YOJ scheduler for a bounded launchd update window.

The LaunchAgent may load before the daily window, when the user logs in during
the window, or while the login keychain is still locked.  This guard only
allows work from 22:30 through 23:30 Beijing time.  A missed day is skipped,
while the scheduler's own checkpoints remain available for the next day's
window.
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


def scheduler_command() -> list[str]:
    return [
        sys.executable,
        str(ROOT / "tools" / "yoj_scheduler.py"),
        "--allow-submit",
        "--publish",
    ]


def run_scheduler() -> int:
    environment = os.environ.copy()
    environment["YOJ_ROOT"] = str(ROOT)
    result = subprocess.run(
        scheduler_command(),
        cwd=ROOT,
        env=environment,
        check=False,
    )
    log(f"调度器结束：exit={result.returncode}")
    return result.returncode


def main() -> int:
    current = now_local()
    if not in_update_window(current):
        log("不在北京时间 22:30–23:30 更新窗口，本次跳过；错过当天不补跑")
        return 0

    state = load_state()
    cycle = cycle_key(current)
    if state.get("lastSuccessfulCycle") == cycle:
        log(f"本周期已完成：cycle={cycle}")
        return 0
    if in_maintenance(current):
        log("处于北京时间 23:55–00:10 维护窗口，等待下一次补偿触发")
        return 0

    STATE_DIR.mkdir(parents=True, exist_ok=True)
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
            log(f"本周期已由其他触发完成：cycle={cycle}")
            return 0
        result = run_scheduler()
        if result == 0:
            state["lastSuccessfulCycle"] = cycle
            state["lastSuccessfulAt"] = now_local().isoformat(timespec="seconds")
            save_state(state)
            log(f"记录本周期成功：cycle={cycle}")
        else:
            log(f"本周期未完成，保留调度器断点等待重试：cycle={cycle}")
        return result


if __name__ == "__main__":
    raise SystemExit(main())
