#!/usr/bin/env python3
"""Run the recoverable YOJ -> local build -> optional publish workflow.

The scheduler is deliberately conservative:

* Beijing maintenance time 23:55--00:10 is a hard stop for network work.
* YOJ credentials are read from environment plus macOS Keychain at runtime.
* Online submission and Git push require explicit flags and environment gates.
* A non-clean/unexpected worktree is never auto-published.
* The lock and reports live under ignored ``.yoj-sync/``.
"""

from __future__ import annotations

import argparse
import fcntl
import os
import subprocess
import sys
from datetime import datetime, time as day_time
from pathlib import Path
from zoneinfo import ZoneInfo


ROOT = Path(os.environ.get("YOJ_ROOT", Path(__file__).resolve().parents[1])).resolve()
STATE_DIR = ROOT / ".yoj-sync"
LOCK_PATH = STATE_DIR / "scheduler.lock"
LOG_PATH = STATE_DIR / "scheduler.log"
KEYCHAIN_SERVICE = "RUC_YOJ/yoj-sync"
TZ = ZoneInfo("Asia/Shanghai")
ALLOWED_PUBLISH_PREFIXES = ("README.md", "data/", "docs/", "题解/", "代码库/")


def now_local() -> datetime:
    return datetime.now(TZ)


def in_maintenance(value: datetime | None = None) -> bool:
    current = (value or now_local()).time()
    start = day_time(23, 55)
    end = day_time(0, 10)
    return current >= start or current < end


def log(message: str) -> None:
    STATE_DIR.mkdir(parents=True, exist_ok=True)
    line = f"{now_local().isoformat(timespec='seconds')} {message}\n"
    with LOG_PATH.open("a", encoding="utf-8") as handle:
        handle.write(line)
    print(message, flush=True)


def run_command(command: list[str], env: dict[str, str] | None = None, timeout: int = 1800) -> int:
    label = " ".join(command)
    log(f"开始：{label}")
    result = subprocess.run(
        command,
        cwd=ROOT,
        env=env,
        capture_output=True,
        text=True,
        timeout=timeout,
        check=False,
    )
    output = (result.stdout + "\n" + result.stderr).strip()
    if output:
        # The child tools never receive the password in their stdout/stderr;
        # retain only a bounded tail so a broken HTML response cannot grow the
        # ignored scheduler log without limit.
        log(f"输出尾部：{output[-2000:]}")
    log(f"完成：{label} -> exit={result.returncode}")
    return result.returncode


def keychain_password(service: str) -> str:
    result = subprocess.run(
        ["security", "find-generic-password", "-s", service, "-w"],
        capture_output=True,
        text=True,
        timeout=15,
        check=False,
    )
    if result.returncode != 0 or not result.stdout.strip():
        raise RuntimeError(f"钥匙串中没有服务 {service!r}；未执行在线提交")
    return result.stdout.strip()


def child_environment() -> dict[str, str]:
    environment = os.environ.copy()
    environment["YOJ_ROOT"] = str(ROOT)
    return environment


def prepare_online_environment() -> dict[str, str]:
    environment = child_environment()
    if not environment.get("YOJ_LOGIN_USER"):
        raise RuntimeError("YOJ_LOGIN_USER 未设置；账号标识应只在本机调度配置中提供")
    environment["YOJ_LOGIN_PASS"] = keychain_password(
        environment.get("YOJ_KEYCHAIN_SERVICE", KEYCHAIN_SERVICE)
    )
    return environment


def changed_paths() -> list[str]:
    result = subprocess.run(
        ["git", "status", "--short", "--untracked-files=all"],
        cwd=ROOT,
        capture_output=True,
        text=True,
        timeout=30,
        check=True,
    )
    paths: list[str] = []
    for line in result.stdout.splitlines():
        if not line:
            continue
        value = line[3:] if len(line) >= 3 else line
        if " -> " in value:
            value = value.split(" -> ", 1)[1]
        paths.append(value.strip())
    return paths


def path_is_allowed(path: str) -> bool:
    return any(path == prefix or path.startswith(prefix) for prefix in ALLOWED_PUBLISH_PREFIXES)


def publish(push: bool) -> int:
    paths = changed_paths()
    unexpected = [path for path in paths if not path_is_allowed(path)]
    if unexpected:
        log(f"发布暂停：工作树含非发布白名单变化 {unexpected[:12]}")
        return 4
    if not paths:
        log("发布跳过：工作树无变化")
        return 0
    check = subprocess.run(["git", "diff", "--check"], cwd=ROOT, capture_output=True, text=True, check=False)
    if check.returncode:
        log(f"发布暂停：git diff --check 失败：{check.stdout[-1000:]}{check.stderr[-1000:]}")
        return check.returncode
    subprocess.run(["git", "add", "--", *ALLOWED_PUBLISH_PREFIXES], cwd=ROOT, check=True)
    staged = subprocess.run(
        ["git", "diff", "--cached", "--name-only"], cwd=ROOT, capture_output=True, text=True, check=True
    ).stdout.splitlines()
    if any(not path_is_allowed(path) for path in staged):
        log("发布暂停：暂存区出现白名单之外的路径")
        subprocess.run(["git", "reset", "--", *ALLOWED_PUBLISH_PREFIXES], cwd=ROOT, check=False)
        return 5
    if not staged:
        log("发布跳过：没有可提交的白名单变化")
        return 0
    message = f"chore: sync YOJ archive {now_local().strftime('%Y-%m-%d %H:%M') }"
    commit = subprocess.run(["git", "commit", "-m", message], cwd=ROOT, capture_output=True, text=True, check=False)
    log(f"Git 提交：exit={commit.returncode} {commit.stdout[-1000:]}{commit.stderr[-1000:]}")
    if commit.returncode:
        return commit.returncode
    if not push:
        log("已生成本地同步提交；未启用 push")
        return 0
    if os.environ.get("YOJ_SYNC_ALLOW_PUSH") != "1":
        log("已生成本地同步提交；YOJ_SYNC_ALLOW_PUSH 不为 1，未推送")
        return 0
    branch = subprocess.run(
        ["git", "branch", "--show-current"], cwd=ROOT, capture_output=True, text=True, check=True
    ).stdout.strip()
    if branch != "main":
        log(f"推送暂停：当前分支为 {branch!r}，只允许明确的 main 调度工作树")
        return 6
    pushed = subprocess.run(["git", "push", "origin", "main"], cwd=ROOT, capture_output=True, text=True, check=False)
    log(f"Git 推送：exit={pushed.returncode} {pushed.stdout[-1000:]}{pushed.stderr[-1000:]}")
    return pushed.returncode


def run_once(args: argparse.Namespace) -> int:
    if in_maintenance():
        log("处于北京时间 23:55–00:10 维护窗口，本轮不访问 YOJ")
        return 0
    STATE_DIR.mkdir(parents=True, exist_ok=True)
    with LOCK_PATH.open("w", encoding="utf-8") as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            log("已有调度实例运行，本轮退出")
            return 0

        environment = child_environment()
        try:
            capture_environment = prepare_online_environment() if not args.dry_run else environment
        except RuntimeError as exc:
            log(f"增量抓取跳过：{exc}")
            return 2
        steps: list[tuple[list[str], dict[str, str], int]] = [
            ([sys.executable, str(ROOT / "tools" / "yoj_capture.py"), "--request-interval", str(args.request_interval)], capture_environment, 3600),
            ([sys.executable, str(ROOT / "tools" / "build_initial.py")], environment, 1800),
            ([sys.executable, str(ROOT / "tools" / "normalize_cpp_headers.py")], environment, 1800),
            ([sys.executable, str(ROOT / "tools" / "port_cpp17_candidates.py")], environment, 1800),
            ([sys.executable, str(ROOT / "tools" / "repair_known_candidates.py")], environment, 1800),
            ([sys.executable, str(ROOT / "tools" / "sanitize_candidates.py")], environment, 1800),
            ([sys.executable, str(ROOT / "tools" / "validate_candidates.py")], environment, 1800),
            ([sys.executable, str(ROOT / "tools" / "compile_candidates.py")], environment, 1800),
            ([sys.executable, str(ROOT / "tools" / "run_samples.py")], environment, 1800),
            ([sys.executable, str(ROOT / "tools" / "audit_forms.py")], environment, 1800),
        ]
        if args.dry_run:
            steps = [([sys.executable, str(ROOT / "tools" / "build_initial.py"), "--check"], environment, 600)]
        for command, step_env, timeout in steps:
            if run_command(command, step_env, timeout):
                log("离线门禁失败，本轮不继续在线提交或发布")
                return 3

        if args.allow_submit:
            if os.environ.get("YOJ_SYNC_ENABLE_SUBMIT") != "1":
                log("在线提交跳过：YOJ_SYNC_ENABLE_SUBMIT 不为 1")
            else:
                try:
                    online_environment = prepare_online_environment()
                except RuntimeError as exc:
                    log(f"在线提交跳过：{exc}")
                else:
                    command = [sys.executable, str(ROOT / "tools" / "online_verify.py"), "--max-submissions", str(args.max_submissions)]
                    if run_command(command, online_environment, 7200):
                        log("在线复验未正常完成；保留 staging 检查点，不发布")
                        return 3
                    if run_command([sys.executable, str(ROOT / "tools" / "build_initial.py")], environment, 1800):
                        return 3

        if args.publish:
            return publish(args.push)
        log("本轮完成：未执行 Git 发布")
        return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="YOJ 抓取、构建、复验和可选 Git 发布调度器")
    parser.add_argument("--allow-submit", action="store_true", help="允许在环境门禁开启时执行在线复验")
    parser.add_argument("--publish", action="store_true", help="提交白名单生成物；不等于推送")
    parser.add_argument("--push", action="store_true", help="在 YOJ_SYNC_ALLOW_PUSH=1 且当前为 main 时推送")
    parser.add_argument("--dry-run", action="store_true", help="只运行离线构建检查，不访问 YOJ")
    parser.add_argument("--max-submissions", type=int, default=int(os.environ.get("YOJ_MAX_SUBMISSIONS", "20")))
    parser.add_argument("--request-interval", type=float, default=float(os.environ.get("YOJ_REQUEST_INTERVAL", "1")))
    args = parser.parse_args()
    if args.max_submissions < 0 or args.request_interval < 0:
        raise SystemExit("提交预算和请求间隔不能为负数")
    if args.dry_run and (args.allow_submit or args.publish or args.push):
        raise SystemExit("--dry-run 不能与在线提交或发布选项同时使用")
    try:
        return run_once(args)
    except (OSError, RuntimeError, subprocess.SubprocessError) as exc:
        log(f"调度异常：{exc.__class__.__name__}: {exc}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
