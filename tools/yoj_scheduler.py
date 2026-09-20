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
import hashlib
import json
import os
import re
import subprocess
import sys
from datetime import datetime, time as day_time
from pathlib import Path
from zoneinfo import ZoneInfo


ROOT = Path(os.environ.get("YOJ_ROOT", Path(__file__).resolve().parents[1])).resolve()
STATE_DIR = ROOT / ".yoj-sync"
LOCK_PATH = STATE_DIR / "scheduler.lock"
LOG_PATH = STATE_DIR / "scheduler.log"
EXPECTED_CHANGES_PATH = STATE_DIR / "expected-generated-changes.json"
CAPTURE_RESULT_PATH = STATE_DIR / "capture-result.json"
PUBLIC_READY_PATH = ROOT / "data" / "public-ready.json"
KEYCHAIN_SERVICE = "RUC_YOJ/yoj-sync"
TZ = ZoneInfo("Asia/Shanghai")
PUBLIC_PUBLISH_PREFIXES = ("README.md", "data/", "docs/")
PROBLEM_PATH_RE = re.compile(r"^(?:代码库|题解)/(\d{4})(?:_|/)")


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


def file_sha256(path: Path) -> str | None:
    if not path.is_file():
        return None
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_expected_changes() -> dict[str, str | None]:
    if not EXPECTED_CHANGES_PATH.is_file():
        return {}
    try:
        payload = json.loads(EXPECTED_CHANGES_PATH.read_text(encoding="utf-8"))
    except (OSError, TypeError, ValueError, json.JSONDecodeError):
        return {}
    return {
        str(item.get("path")): item.get("sha256")
        for item in (payload.get("paths") or [])
        if item.get("path")
    }


def remember_generated_changes() -> None:
    """Remember scheduler-created dirty files without treating them as trusted forever."""

    paths = changed_paths()
    payload = {
        "schemaVersion": 1,
        "purpose": "hashes of scheduler-created pending changes; not a publication manifest",
        "paths": [{"path": path, "sha256": file_sha256(ROOT / path)} for path in paths],
    }
    STATE_DIR.mkdir(parents=True, exist_ok=True)
    EXPECTED_CHANGES_PATH.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def load_capture_result() -> dict[str, object] | None:
    """Read the current capture hand-off; fail closed if it is unavailable."""

    if not CAPTURE_RESULT_PATH.is_file():
        return None
    try:
        payload = json.loads(CAPTURE_RESULT_PATH.read_text(encoding="utf-8"))
    except (OSError, TypeError, ValueError, json.JSONDecodeError):
        return None
    return payload if isinstance(payload, dict) else None


def problem_selector(problem_numbers: list[int]) -> list[str]:
    return [argument for problem_no in problem_numbers for argument in ("--problem", str(problem_no))]


def validate_worktree_before_run() -> bool:
    """Allow only unchanged pending output from an earlier scheduler run."""

    paths = changed_paths()
    if not paths:
        return True
    expected = load_expected_changes()
    unexpected = [path for path in paths if path not in expected or expected[path] != file_sha256(ROOT / path)]
    missing = [path for path in expected if path not in paths]
    if unexpected or missing:
        log(
            "调度暂停：工作树存在未获本地执行器确认的变化 "
            f"{(unexpected + missing)[:12]}；请先人工审阅并处理"
        )
        return False
    log(f"恢复上轮待处理输出：{len(paths)} 个文件，哈希与检查点一致")
    return True


def public_ready_snapshot() -> dict[int, str]:
    if not PUBLIC_READY_PATH.is_file():
        return {}
    try:
        payload = json.loads(PUBLIC_READY_PATH.read_text(encoding="utf-8"))
    except (OSError, TypeError, ValueError, json.JSONDecodeError):
        return {}
    return {
        int(item["problemNo"]): json.dumps(item, ensure_ascii=False, sort_keys=True)
        for item in (payload.get("records") or [])
        if item.get("status") == "PUBLIC_READY" and str(item.get("problemNo", "")).isdigit()
    }


def public_ready_problem_numbers() -> set[int]:
    return set(public_ready_snapshot())


def path_problem_number(path: str) -> int | None:
    match = PROBLEM_PATH_RE.match(path)
    return int(match.group(1)) if match else None


def path_is_publishable(path: str, release_numbers: set[int]) -> bool:
    if any(path == prefix or path.startswith(prefix) for prefix in PUBLIC_PUBLISH_PREFIXES):
        return True
    problem_no = path_problem_number(path)
    return problem_no is not None and problem_no in release_numbers


def scan_for_secrets(paths: list[str], secret_values: tuple[str, ...]) -> str | None:
    needles = [value.encode("utf-8") for value in secret_values if value and len(value) >= 4]
    if not needles:
        return None
    for path in paths:
        candidate = ROOT / path
        if not candidate.is_file():
            continue
        data = candidate.read_bytes()
        if any(needle in data for needle in needles):
            return path
    return None


def public_ready_delta(before: dict[int, str]) -> set[int]:
    after = public_ready_snapshot()
    return {problem_no for problem_no, value in after.items() if before.get(problem_no) != value}


def publish(push: bool, release_numbers: set[int], secret_values: tuple[str, ...] = ()) -> int:
    paths = changed_paths()
    secret_path = scan_for_secrets(paths, secret_values)
    if secret_path:
        log(f"发布暂停：文件 {secret_path} 命中运行时凭据；未暂存、未提交、未推送")
        return 7
    if not paths:
        log("发布跳过：工作树无变化")
        return 0
    check_paths = [
        path
        for path in paths
        if any(path == prefix or path.startswith(prefix) for prefix in PUBLIC_PUBLISH_PREFIXES)
    ]
    check_command = ["git", "diff", "--check", "--", *check_paths] if check_paths else ["git", "diff", "--check"]
    check = subprocess.run(check_command, cwd=ROOT, capture_output=True, text=True, check=False)
    if check.returncode:
        log(f"发布暂停：git diff --check 失败：{check.stdout[-1000:]}{check.stderr[-1000:]}")
        return check.returncode
    stage_paths = [path for path in paths if path_is_publishable(path, release_numbers)]
    deferred = [path for path in paths if path not in stage_paths]
    if deferred:
        log(f"发布保留本地待处理变化（未暂存）：{deferred[:12]}")
    if not stage_paths:
        log("发布跳过：没有本轮 PUBLIC_READY 对应或公开索引白名单变化")
        return 0
    subprocess.run(["git", "add", "--", *stage_paths], cwd=ROOT, check=True)
    staged = subprocess.run(
        ["git", "diff", "--cached", "--name-only"], cwd=ROOT, capture_output=True, text=True, check=True
    ).stdout.splitlines()
    if any(not path_is_publishable(path, release_numbers) for path in staged):
        log("发布暂停：暂存区出现白名单之外的路径")
        subprocess.run(["git", "reset", "--", *staged], cwd=ROOT, check=False)
        return 5
    staged_secret_path = scan_for_secrets(staged, secret_values)
    if staged_secret_path:
        log(f"发布暂停：暂存文件 {staged_secret_path} 命中运行时凭据；已取消暂存")
        subprocess.run(["git", "reset", "--", *staged], cwd=ROOT, check=False)
        return 7
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

        if not validate_worktree_before_run():
            return 4

        environment = child_environment()
        online_environment: dict[str, str] | None = None
        online_ran = False
        release_numbers: set[int] = set()
        if args.dry_run:
            offline_checks = [
                [sys.executable, str(ROOT / "tools" / "build_initial.py"), "--check"],
                [sys.executable, str(ROOT / "tools" / "audit_online_availability.py"), "--check"],
                [sys.executable, str(ROOT / "tools" / "build_site_catalog.py"), "--check"],
                [sys.executable, str(ROOT / "tools" / "audit_consistency.py")],
            ]
            for command in offline_checks:
                if run_command(command, environment, 600):
                    log("dry-run 离线检查失败")
                    return 3
            remember_generated_changes()
            log("dry-run 完成：未访问 YOJ，未执行在线复验或发布")
            return 0
        # Refresh the public, unauthenticated evidence first.  This keeps the
        # visibility axis current even when no new problem is discovered, and
        # ensures the account is never needed merely to compare problem IDs.
        if run_command(
            [sys.executable, str(ROOT / "tools" / "audit_online_availability.py")],
            environment,
            600,
        ):
            log("公开题目列表快照未正常完成，本轮不继续抓取、构建或发布")
            return 3

        capture_environment = environment
        public_only = False
        try:
            capture_environment = prepare_online_environment()
        except RuntimeError as exc:
            # A public-list discovery must still be useful without an account:
            # capture the new statements and let later stages mark them as
            # TOPIC_CAPTURED/NO_LOCAL_AC instead of dropping the update.
            public_only = True
            log(f"未取得 YOJ 登录态，改用公开题面增量抓取：{exc}")
        capture_command = [
            sys.executable,
            str(ROOT / "tools" / "yoj_capture.py"),
            "--request-interval",
            str(args.request_interval),
            "--public-snapshot",
            str(ROOT / "data" / "yoj-public-problems.json"),
        ]
        if public_only:
            capture_command.append("--public-only")
        if run_command(capture_command, capture_environment, 3600):
            log("增量抓取未正常完成，本轮不继续构建、复验或发布")
            return 3
        capture_result = load_capture_result()
        if not capture_result or capture_result.get("scope") != "public_problem_numbers_only":
            log("调度暂停：抓取结果缺失或版本不受信，未继续后续阶段")
            remember_generated_changes()
            return 3
        try:
            new_problem_numbers = sorted({int(value) for value in (capture_result.get("newProblemNumbers") or [])})
        except (TypeError, ValueError):
            log("调度暂停：抓取结果中的新题号无法解析")
            remember_generated_changes()
            return 3
        if not new_problem_numbers:
            log("本轮未发现本地从未出现过的新题号；跳过整库构建、复验和发布")
            remember_generated_changes()
            return 0

        selected = problem_selector(new_problem_numbers)
        log(f"本轮只处理新题号：{new_problem_numbers}")
        steps: list[tuple[list[str], dict[str, str], int]] = [
            (
                [sys.executable, str(ROOT / "tools" / "audit_online_availability.py")],
                environment,
                600,
            ),
            (
                [sys.executable, str(ROOT / "tools" / "build_initial.py"), "--preserve-frozen", *selected],
                environment,
                1800,
            ),
            ([sys.executable, str(ROOT / "tools" / "normalize_cpp_headers.py"), *selected], environment, 1800),
            ([sys.executable, str(ROOT / "tools" / "port_cpp17_candidates.py"), *selected], environment, 1800),
            ([sys.executable, str(ROOT / "tools" / "repair_known_candidates.py"), *selected], environment, 1800),
            ([sys.executable, str(ROOT / "tools" / "sanitize_candidates.py"), *selected], environment, 1800),
            ([sys.executable, str(ROOT / "tools" / "validate_candidates.py"), *selected], environment, 1800),
            ([sys.executable, str(ROOT / "tools" / "compile_candidates.py"), *selected], environment, 1800),
            ([sys.executable, str(ROOT / "tools" / "run_samples.py"), *selected], environment, 1800),
            ([sys.executable, str(ROOT / "tools" / "audit_forms.py"), *selected], environment, 1800),
            ([sys.executable, str(ROOT / "tools" / "audit_consistency.py")], environment, 600),
        ]
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
                    command = [
                        sys.executable,
                        str(ROOT / "tools" / "online_verify.py"),
                        "--max-submissions",
                        str(args.max_submissions),
                        *selected,
                    ]
                    if run_command(command, online_environment, 7200):
                        log("在线复验未正常完成；保留 staging 检查点，不发布")
                        remember_generated_changes()
                        return 3
                    online_ran = True

        if args.publish:
            before_ready = public_ready_snapshot()
            if run_command([sys.executable, str(ROOT / "tools" / "publish_ready.py"), "--apply"], environment, 1800):
                log("公开发布闸门未正常完成；不生成 Git 提交")
                remember_generated_changes()
                return 3
            release_numbers = public_ready_delta(before_ready)
            # A newly discovered topic must be publishable even when it has no
            # local AC yet; otherwise the catalog would link to files that were
            # deliberately left out of the commit.
            release_numbers.update(new_problem_numbers)
            if run_command(
                [sys.executable, str(ROOT / "tools" / "build_initial.py"), "--preserve-frozen", *selected],
                environment,
                1800,
            ):
                remember_generated_changes()
                return 3
            if run_command([sys.executable, str(ROOT / "tools" / "build_site_catalog.py")], environment, 600):
                remember_generated_changes()
                return 3
            if run_command([sys.executable, str(ROOT / "tools" / "build_site_catalog.py"), "--check"], environment, 600):
                remember_generated_changes()
                return 3
            if run_command([sys.executable, str(ROOT / "tools" / "audit_consistency.py")], environment, 600):
                remember_generated_changes()
                return 3
            result = publish(
                args.push,
                release_numbers,
                tuple(
                    value
                    for value in (
                        (online_environment or {}).get("YOJ_LOGIN_USER"),
                        (online_environment or {}).get("YOJ_LOGIN_PASS"),
                    )
                    if value
                ),
            )
            remember_generated_changes()
            return result
        if online_ran:
            if run_command(
                [sys.executable, str(ROOT / "tools" / "build_initial.py"), "--preserve-frozen", *selected],
                environment,
                1800,
            ):
                remember_generated_changes()
                return 3
        if not args.dry_run:
            if run_command([sys.executable, str(ROOT / "tools" / "build_site_catalog.py")], environment, 600):
                remember_generated_changes()
                return 3
            if run_command([sys.executable, str(ROOT / "tools" / "build_site_catalog.py"), "--check"], environment, 600):
                remember_generated_changes()
                return 3
            if run_command([sys.executable, str(ROOT / "tools" / "audit_consistency.py")], environment, 600):
                remember_generated_changes()
                return 3
        remember_generated_changes()
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
