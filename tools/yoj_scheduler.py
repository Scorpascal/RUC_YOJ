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
import shutil
import subprocess
import sys
import tarfile
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime, time as day_time
from pathlib import Path
from zoneinfo import ZoneInfo


ROOT = Path(os.environ.get("YOJ_ROOT", Path(__file__).resolve().parents[1])).resolve()
STATE_DIR = ROOT / ".yoj-sync"
LOCK_PATH = STATE_DIR / "scheduler.lock"
LOG_PATH = STATE_DIR / "scheduler.log"
EXPECTED_CHANGES_PATH = STATE_DIR / "expected-generated-changes.json"
PENDING_REMOTE_PATH = STATE_DIR / "pending-remote-deployment.json"
CAPTURE_RESULT_PATH = STATE_DIR / "capture-result.json"
VISIBILITY_REPORT_PATH = STATE_DIR / "public-visibility-report.json"
SENTINEL_STATE_PATH = STATE_DIR / "sentinel-state.json"
DRIFT_REPORT_PATH = ROOT / "staging" / "problem-drift.json"
PUBLIC_READY_PATH = ROOT / "data" / "public-ready.json"
PROBLEMS_PATH = ROOT / "data" / "problems.json"
PUBLIC_SNAPSHOT_PATH = ROOT / "data" / "yoj-public-problems.json"
KEYCHAIN_SERVICE = "RUC_YOJ/yoj-sync"
TZ = ZoneInfo("Asia/Shanghai")
PUBLIC_PUBLISH_PREFIXES = ("README.md", "data/", "docs/")
PROBLEM_PATH_RE = re.compile(r"^(?:代码库|题解)/(\d{4})(?:_|/)")
PAGES_WORKFLOW_PATH = ".github/workflows/pages.yml"
REMOTE_VERIFY_TIMEOUT = max(30, int(os.environ.get("YOJ_SYNC_REMOTE_VERIFY_TIMEOUT", "180")))


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
    """Return actual repository-relative paths without Git's C-style quoting.

    ``git status --short`` quotes non-ASCII names by default.  That is unsafe
    here because the publication allowlist and the problem-number matcher use
    the real UTF-8 path (``代码库/`` and ``题解/``).  Porcelain ``-z`` is the
    machine-readable form: filenames are emitted as raw bytes and are never
    quoted.  Rename/copy records contain a second NUL-terminated path; retain
    both names so staging and the resume checkpoint cannot silently lose one
    side of a change.
    """
    result = subprocess.run(
        ["git", "status", "--porcelain=v1", "--untracked-files=all", "-z"],
        cwd=ROOT,
        capture_output=True,
        text=False,
        timeout=30,
        check=True,
    )
    paths: list[str] = []
    records = result.stdout.split(b"\0")
    index = 0
    while index < len(records):
        record = records[index]
        index += 1
        if not record:
            continue
        if len(record) < 3:
            raise RuntimeError("git status returned a malformed porcelain record")
        status = record[:2].decode("ascii", errors="replace")
        path = os.fsdecode(record[3:])
        if path:
            paths.append(path)
        if "R" in status or "C" in status:
            if index >= len(records) or not records[index]:
                raise RuntimeError("git status returned an incomplete rename/copy record")
            paths.append(os.fsdecode(records[index]))
            index += 1
    return paths


def staged_paths() -> list[str]:
    """Return the paths in the index using Git's unquoted NUL format."""

    result = subprocess.run(
        ["git", "diff", "--cached", "--name-only", "-z"],
        cwd=ROOT,
        capture_output=True,
        text=False,
        timeout=30,
        check=True,
    )
    return [os.fsdecode(value) for value in result.stdout.split(b"\0") if value]


def git_head_sha() -> str:
    result = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=ROOT,
        capture_output=True,
        text=True,
        timeout=30,
        check=True,
    )
    return result.stdout.strip()


def github_repository() -> tuple[str, str] | None:
    """Return the owner/repository for the origin GitHub remote."""

    result = subprocess.run(
        ["git", "remote", "get-url", "origin"],
        cwd=ROOT,
        capture_output=True,
        text=True,
        timeout=30,
        check=False,
    )
    if result.returncode:
        return None
    remote = result.stdout.strip()
    if remote.startswith("git@github.com:"):
        slug = remote.split(":", 1)[1]
    else:
        parsed = urllib.parse.urlparse(remote)
        if parsed.hostname != "github.com":
            return None
        slug = parsed.path.strip("/")
    if slug.endswith(".git"):
        slug = slug[:-4]
    parts = [part for part in slug.split("/") if part]
    return (parts[0], parts[1]) if len(parts) == 2 else None


def github_api_json(endpoint: str) -> tuple[object | None, str | None]:
    """Read a public GitHub API response with a verified-TLS fallback.

    The launchd interpreter may be a framework/conda Python whose OpenSSL CA
    bundle is absent or stale, while macOS ``curl`` uses SecureTransport and
    the system trust store.  Retry the same read-only request through curl;
    never disable certificate verification and never pass the YOJ credential.
    """

    request = urllib.request.Request(
        endpoint,
        headers={
            "Accept": "application/vnd.github+json",
            "User-Agent": "RUC-YOJ-sync/1.0",
        },
    )
    try:
        with urllib.request.urlopen(request, timeout=15) as response:
            return json.loads(response.read().decode("utf-8")), None
    except (OSError, urllib.error.URLError, urllib.error.HTTPError, UnicodeError, json.JSONDecodeError) as exc:
        urllib_detail = str(exc)

    curl = shutil.which("curl")
    if not curl:
        return None, f"Python HTTPS: {urllib_detail}; curl 不可用"
    fallback = subprocess.run(
        [
            curl,
            "--fail",
            "--silent",
            "--show-error",
            "--location",
            "--max-time",
            "15",
            "-H",
            "Accept: application/vnd.github+json",
            "-H",
            "User-Agent: RUC-YOJ-sync/1.0",
            endpoint,
        ],
        cwd=ROOT,
        capture_output=True,
        text=True,
        timeout=20,
        check=False,
    )
    if fallback.returncode:
        curl_detail = (fallback.stderr or fallback.stdout).strip()[-500:]
        return None, f"Python HTTPS: {urllib_detail}; curl: {curl_detail or fallback.returncode}"
    try:
        return json.loads(fallback.stdout), None
    except (TypeError, ValueError, json.JSONDecodeError) as exc:
        return None, f"Python HTTPS: {urllib_detail}; curl JSON: {exc}"


def load_pending_remote() -> dict[str, object] | None:
    if not PENDING_REMOTE_PATH.is_file():
        return None
    try:
        payload = json.loads(PENDING_REMOTE_PATH.read_text(encoding="utf-8"))
    except (OSError, TypeError, ValueError, json.JSONDecodeError):
        return None
    return payload if isinstance(payload, dict) else None


def save_pending_remote(commit_sha: str, state: str = "pending", detail: str = "") -> None:
    STATE_DIR.mkdir(parents=True, exist_ok=True)
    payload = {
        "schemaVersion": 1,
        "commit": commit_sha,
        "workflow": PAGES_WORKFLOW_PATH,
        "state": state,
        "detail": detail,
        "updatedAt": now_local().isoformat(timespec="seconds"),
    }
    temporary = PENDING_REMOTE_PATH.with_name(PENDING_REMOTE_PATH.name + ".tmp")
    temporary.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    temporary.replace(PENDING_REMOTE_PATH)


def clear_pending_remote() -> None:
    try:
        PENDING_REMOTE_PATH.unlink()
    except FileNotFoundError:
        pass


def remote_workflow_state(commit_sha: str) -> tuple[str, str]:
    """Read the public GitHub Actions state for one pushed commit.

    The repository is public, so this read-only API check does not require a
    token.  ``MISSING`` and ``UNAVAILABLE`` are deliberately distinct from a
    terminal workflow failure: the former may simply mean GitHub has not
    queued the push yet.
    """

    repository = github_repository()
    if not repository:
        return "UNAVAILABLE", "origin is not a GitHub repository"
    owner, name = repository
    query = urllib.parse.urlencode({"head_sha": commit_sha, "per_page": "20"})
    endpoint = f"https://api.github.com/repos/{owner}/{name}/actions/runs?{query}"
    payload, error = github_api_json(endpoint)
    if error:
        return "UNAVAILABLE", f"GitHub Actions API: {error}"
    runs = payload.get("workflow_runs") if isinstance(payload, dict) else None
    if not isinstance(runs, list):
        return "UNAVAILABLE", "GitHub Actions API returned no workflow_runs list"
    matching = [
        run
        for run in runs
        if isinstance(run, dict)
        and str(run.get("head_sha") or "") == commit_sha
        and str(run.get("path") or "") == PAGES_WORKFLOW_PATH
    ]
    if not matching:
        return "MISSING", "no Pages workflow run for the pushed commit"
    run = max(matching, key=lambda item: int(item.get("id") or 0))
    status = str(run.get("status") or "unknown")
    url = str(run.get("html_url") or "")
    if status != "completed":
        return "RUNNING", f"{status} {url}".strip()
    conclusion = str(run.get("conclusion") or "unknown")
    if conclusion == "success":
        return "SUCCESS", url
    return "FAILURE", f"{conclusion} {url}".strip()


def verify_remote_workflow(commit_sha: str) -> bool:
    """Wait for the Pages workflow for a pushed commit to reach success."""

    deadline = time.monotonic() + REMOTE_VERIFY_TIMEOUT
    while True:
        state, detail = remote_workflow_state(commit_sha)
        if state == "SUCCESS":
            log(f"GitHub Pages workflow 已成功：commit={commit_sha[:12]} {detail}")
            return True
        if state == "FAILURE":
            log(f"GitHub Pages workflow 失败：commit={commit_sha[:12]} {detail}")
            return False
        if time.monotonic() >= deadline:
            log(f"GitHub Pages workflow 未在时限内完成：commit={commit_sha[:12]} state={state} {detail}")
            return False
        log(f"等待 GitHub Pages workflow：commit={commit_sha[:12]} state={state} {detail}")
        time.sleep(min(15, max(1, int(deadline - time.monotonic()))))


def verify_pending_remote() -> bool:
    pending = load_pending_remote()
    if not pending:
        return True
    commit_sha = str(pending.get("commit") or "")
    if not re.fullmatch(r"[0-9a-f]{40}", commit_sha):
        log("调度暂停：远端部署检查点损坏；未继续自动发布")
        return False
    log(f"恢复远端部署检查点：commit={commit_sha[:12]}")
    if verify_remote_workflow(commit_sha):
        clear_pending_remote()
        return True
    save_pending_remote(commit_sha, state="failed-or-pending")
    return False


def file_sha256(path: Path) -> str | None:
    if not path.is_file():
        return None
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def decode_legacy_git_status_path(value: str) -> str:
    """Decode the C-style path emitted by the old text status parser."""

    if len(value) < 2 or value[0] != '"' or value[-1] != '"':
        return value
    encoded = value[1:-1]
    raw = bytearray()
    index = 0
    while index < len(encoded):
        character = encoded[index]
        if character != "\\":
            raw.extend(character.encode("utf-8"))
            index += 1
            continue
        index += 1
        if index >= len(encoded):
            raw.extend(b"\\")
            break
        escaped = encoded[index]
        index += 1
        if escaped in "01234567":
            digits = escaped
            while index < len(encoded) and len(digits) < 3 and encoded[index] in "01234567":
                digits += encoded[index]
                index += 1
            raw.append(int(digits, 8))
        else:
            raw.extend({"t": b"\t", "n": b"\n", "r": b"\r"}.get(escaped, escaped.encode("utf-8")))
    return os.fsdecode(bytes(raw))


def snapshot_content_digest(path: Path) -> str | None:
    """Return the public-list digest, ignoring a timestamp-only rewrite."""

    if not path.is_file():
        return None
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, TypeError, ValueError, json.JSONDecodeError):
        return file_sha256(path)
    digest = payload.get("problemListSha256") if isinstance(payload, dict) else None
    return str(digest) if digest else file_sha256(path)


def load_expected_changes() -> dict[str, str | None]:
    if not EXPECTED_CHANGES_PATH.is_file():
        return {}
    try:
        payload = json.loads(EXPECTED_CHANGES_PATH.read_text(encoding="utf-8"))
    except (OSError, TypeError, ValueError, json.JSONDecodeError):
        return {}
    return {
        decode_legacy_git_status_path(str(item.get("path"))): item.get("sha256")
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


def load_visibility_report() -> dict[str, object] | None:
    """Read the current public-list transition report."""

    if not VISIBILITY_REPORT_PATH.is_file():
        return None
    try:
        payload = json.loads(VISIBILITY_REPORT_PATH.read_text(encoding="utf-8"))
    except (OSError, TypeError, ValueError, json.JSONDecodeError):
        return None
    return payload if isinstance(payload, dict) else None


def run_visibility_audit(environment: dict[str, str], commit: bool = False) -> bool:
    command = [
        sys.executable,
        str(ROOT / "tools" / "audit_public_visibility.py"),
        "--snapshot",
        str(PUBLIC_SNAPSHOT_PATH),
        "--report",
        str(VISIBILITY_REPORT_PATH),
    ]
    if commit:
        command.append("--commit")
    return run_command(command, environment, 600) == 0


def problem_selector(problem_numbers: list[int]) -> list[str]:
    return [argument for problem_no in problem_numbers for argument in ("--problem", str(problem_no))]


def validate_worktree_before_run() -> bool:
    """Allow only unchanged pending output from an earlier scheduler run."""

    paths = changed_paths()
    if not paths:
        return True
    expected = load_expected_changes()
    legacy_checkpoint = any(value is None for value in expected.values())
    if legacy_checkpoint:
        unexpected = [path for path in paths if path not in expected]
        if unexpected:
            log(
                "调度暂停：旧版路径检查点无法覆盖当前变化 "
                f"{unexpected[:12]}；请先人工审阅并处理"
            )
            return False
        # The old parser stored quoted paths and null hashes, so it cannot
        # prove byte identity.  Rebaseline only the exact known path set once;
        # any newly appearing path still fails closed above.
        log("迁移旧版路径检查点：改用真实 UTF-8 路径并重新记录当前文件哈希")
        remember_generated_changes()
        return True
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


def select_raw_cleanup_backlog() -> list[int]:
    """Select every RAW_CAPTURED archived AC that still needs release.

    New-topic discovery is intentionally separate from this backlog.  A topic
    can be present in the local archive for days while its raw Accepted source
    is still waiting for sanitization, local gates, or a fresh online
    round-trip.  The backlog is deliberately independent of the current YOJ
    public-list snapshot: a previously public problem must not be stranded in
    RAW_CAPTURED merely because it later disappears from that snapshot.  Only
    candidates with both archived source files and an Accepted archive run are
    eligible; topic-only records never enter an unattended submission path.
    """

    try:
        problems_payload = json.loads(PROBLEMS_PATH.read_text(encoding="utf-8"))
    except (OSError, TypeError, ValueError, json.JSONDecodeError) as exc:
        log(f"清洗 backlog 选择失败：{exc}")
        return []

    frozen_public = public_ready_problem_numbers()
    selected: list[int] = []
    for record in problems_payload.get("records") or []:
        if not isinstance(record, dict) or not str(record.get("problemNo", "")).isdigit():
            continue
        problem_no = int(record["problemNo"])
        if problem_no in frozen_public:
            continue
        archive = record.get("archive") or {}
        public = record.get("public") or {}
        accepted_run = archive.get("acceptedRun") or {}
        complete = ROOT / str(archive.get("completeCode") or "")
        direct = ROOT / str(archive.get("directlySubmittableCode") or "")
        if not complete.is_file() or not direct.is_file():
            continue
        if str(accepted_run.get("status") or "").strip().lower() != "accepted":
            continue
        if (
            str(public.get("status") or "") == "RAW_CAPTURED"
            or str(public.get("onlineVerification") or "") != "ONLINE_ACCEPTED"
        ):
            selected.append(problem_no)
    return sorted(set(selected))


def load_sentinel_state() -> dict[str, object]:
    if not SENTINEL_STATE_PATH.is_file():
        return {}
    try:
        payload = json.loads(SENTINEL_STATE_PATH.read_text(encoding="utf-8"))
    except (OSError, TypeError, ValueError, json.JSONDecodeError):
        return {}
    return payload if isinstance(payload, dict) else {}


def select_sentinel_problems(count: int) -> tuple[list[int], int]:
    """Select a deterministic rotating slice of frozen public solutions."""

    candidates = sorted(public_ready_problem_numbers())
    if not candidates or count <= 0:
        return [], 0
    state = load_sentinel_state()
    try:
        cursor = int(state.get("cursor", 0)) % len(candidates)
    except (TypeError, ValueError):
        cursor = 0
    amount = min(count, len(candidates))
    selected = [candidates[(cursor + offset) % len(candidates)] for offset in range(amount)]
    return selected, (cursor + amount) % len(candidates)


def save_sentinel_state(problem_numbers: list[int], next_cursor: int) -> None:
    STATE_DIR.mkdir(parents=True, exist_ok=True)
    payload = {
        "schemaVersion": 1,
        "purpose": "rotating low-frequency YOJ online sentinel cursor; no release state",
        "cursor": next_cursor,
        "lastProblemNumbers": problem_numbers,
        "lastRunAt": now_local().isoformat(timespec="seconds"),
    }
    temporary = SENTINEL_STATE_PATH.with_name(SENTINEL_STATE_PATH.name + ".tmp")
    temporary.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    temporary.replace(SENTINEL_STATE_PATH)


def sentinel_all_accepted(problem_numbers: list[int]) -> tuple[bool, dict[int, str]]:
    """Treat a completed command as success only when every probe is Accepted."""

    report_path = ROOT / "staging" / "online-sentinel.json"
    try:
        payload = json.loads(report_path.read_text(encoding="utf-8"))
    except (OSError, TypeError, ValueError, json.JSONDecodeError):
        return False, {problem_no: "REPORT_MISSING_OR_INVALID" for problem_no in problem_numbers}
    records = {
        int(row["problemNo"]): str(row.get("status") or "UNKNOWN")
        for row in (payload.get("records") or [])
        if str(row.get("problemNo", "")).isdigit()
    }
    skipped = {
        int(row["problemNo"]): str(row.get("reason") or "SKIPPED")
        for row in (payload.get("skipped") or [])
        if str(row.get("problemNo", "")).isdigit()
    }
    outcomes = {
        problem_no: records.get(problem_no, skipped.get(problem_no, "NO_RESULT"))
        for problem_no in problem_numbers
    }
    return all(outcome == "Accepted" for outcome in outcomes.values()), outcomes


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


def run_staged_consistency_audit() -> bool:
    """Audit the exact index tree that the next commit would contain.

    The ordinary audit reads the working tree.  That is necessary for the
    build stages, but it cannot catch a publisher that accidentally leaves a
    referenced source file unstaged.  Materialising the index with
    ``git write-tree``/``git archive`` makes the final commit boundary
    independently auditable before ``git commit``.
    """

    tree = subprocess.run(
        ["git", "write-tree"],
        cwd=ROOT,
        capture_output=True,
        text=True,
        timeout=30,
        check=False,
    )
    if tree.returncode or not tree.stdout.strip():
        log(f"发布暂停：无法生成暂存树：{tree.stdout[-500:]}{tree.stderr[-500:]}")
        return False

    with tempfile.TemporaryDirectory(prefix="yoj-staged-audit-") as directory:
        temporary_root = Path(directory)
        archive_path = temporary_root / "index.tar"
        staged_root = (temporary_root / "tree").resolve()
        staged_root.mkdir()
        archived = subprocess.run(
            [
                "git",
                "archive",
                "--format=tar",
                "--output",
                str(archive_path),
                tree.stdout.strip(),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
            timeout=120,
            check=False,
        )
        if archived.returncode:
            log(f"发布暂停：无法展开暂存树：{archived.stdout[-500:]}{archived.stderr[-500:]}")
            return False
        try:
            with tarfile.open(archive_path, mode="r") as archive:
                for member in archive.getmembers():
                    target = (staged_root / member.name).resolve()
                    if target != staged_root and staged_root not in target.parents:
                        log(f"发布暂停：暂存树包含越界路径：{member.name}")
                        return False
                    archive.extract(member, staged_root)
        except (OSError, tarfile.TarError) as exc:
            log(f"发布暂停：展开暂存树失败：{exc}")
            return False

        audit = subprocess.run(
            [sys.executable, str(staged_root / "tools" / "audit_consistency.py")],
            cwd=staged_root,
            env=child_environment(),
            capture_output=True,
            text=True,
            timeout=600,
            check=False,
        )
        output = (audit.stdout + "\n" + audit.stderr).strip()
        if output:
            log(f"暂存树一致性审计输出尾部：{output[-2000:]}")
        if audit.returncode:
            log("发布暂停：最终暂存树未通过一致性审计；未提交、未推送")
            return False
    return True


def public_ready_delta(before: dict[int, str]) -> set[int]:
    after = public_ready_snapshot()
    return {problem_no for problem_no, value in after.items() if before.get(problem_no) != value}


def run_visibility_watch(args: argparse.Namespace, environment: dict[str, str]) -> int:
    """Recheck and publish only the public-list projection after the main sync.

    launchd invokes this mode on its 15-minute compensation interval after
    the full daily cycle has succeeded.  It must never discover/download code,
    run candidate gates, or submit to YOJ.
    """

    snapshot_before = snapshot_content_digest(PUBLIC_SNAPSHOT_PATH)
    if run_command(
        [sys.executable, str(ROOT / "tools" / "audit_online_availability.py")],
        environment,
        600,
    ):
        log("轻量可见性复查未能获取公开列表；保留基线，等待下一次间隔复查")
        return 3
    snapshot_changed = snapshot_content_digest(PUBLIC_SNAPSHOT_PATH) != snapshot_before
    if not run_visibility_audit(environment):
        log("轻量可见性复查无法生成状态报告；保留基线，等待下一次间隔复查")
        return 3
    report = load_visibility_report()
    if not report:
        log("轻量可见性复查报告缺失；保留基线，等待下一次间隔复查")
        return 3

    transitions = bool(report.get("hasVisibilityTransitions"))
    if snapshot_changed or transitions:
        reopened = report.get("reopenedProblemNumbers") or []
        archived = report.get("archivedProblemNumbers") or []
        log(
            "轻量复查发现公开列表变化："
            f"重新开放 {reopened}，下线 {archived}；刷新 README 和 Pages 目录"
        )
        if run_command(
            [sys.executable, str(ROOT / "tools" / "build_initial.py"), "--visibility-only"],
            environment,
            1800,
        ):
            log("轻量可见性投影失败；保留旧基线，等待下一次间隔复查")
            remember_generated_changes()
            return 3
        for command in (
            [sys.executable, str(ROOT / "tools" / "build_site_catalog.py"), "--check"],
            [sys.executable, str(ROOT / "tools" / "audit_consistency.py")],
        ):
            if run_command(command, environment, 600):
                log("轻量可见性投影未通过一致性门禁；保留旧基线，等待下一次间隔复查")
                remember_generated_changes()
                return 3
        result = publish(args.push, set()) if args.publish else 0
        if result:
            log("轻量可见性投影发布未完成；保留旧基线，等待下一次间隔复查")
            remember_generated_changes()
            return result
    else:
        log("轻量可见性复查完成：公开列表未变化")

    if not run_visibility_audit(environment, commit=True):
        log("轻量可见性基线保存失败；等待下一次间隔复查")
        remember_generated_changes()
        return 3
    remember_generated_changes()
    return 0


def publish(push: bool, release_numbers: set[int], secret_values: tuple[str, ...] = ()) -> int:
    preexisting_staged = staged_paths()
    if preexisting_staged:
        log(
            "发布暂停：检测到调度器启动前已有暂存内容；"
            f"为避免误提交人工变化，未继续发布：{preexisting_staged[:12]}"
        )
        return 5
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
    staged = staged_paths()
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
    if not run_staged_consistency_audit():
        subprocess.run(["git", "reset", "--", *staged], cwd=ROOT, check=False)
        return 5
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
    if pushed.returncode:
        return pushed.returncode
    commit_sha = git_head_sha()
    save_pending_remote(commit_sha)
    if not verify_remote_workflow(commit_sha):
        return 8
    clear_pending_remote()
    return 0


def run_drift_audit(args: argparse.Namespace, environment: dict[str, str]) -> int:
    """Run the explicit, low-frequency semantic statement audit only."""

    # Refreshing this unauthenticated list is part of the low-frequency audit
    # boundary.  It is deliberately not part of the ordinary no-new daily
    # path beyond the existing ID discovery.
    if run_command(
        [sys.executable, str(ROOT / "tools" / "audit_online_availability.py")],
        environment,
        600,
    ):
        log("低频漂移审计暂停：YOJ 当前公开列表快照未正常完成")
        return 3
    command = [
        sys.executable,
        str(ROOT / "tools" / "audit_problem_drift.py"),
        "--public-snapshot",
        str(PUBLIC_SNAPSHOT_PATH),
        "--report",
        str(DRIFT_REPORT_PATH),
        "--request-interval",
        str(args.drift_request_interval),
    ]
    if args.drift_max_problems:
        command.extend(["--max-problems", str(args.drift_max_problems)])
    result = run_command(command, environment, max(1800, args.drift_max_problems * 45 or 1800))
    remember_generated_changes()
    if result:
        log("低频漂移审计发现抓取/基线异常；现有 PUBLIC_READY 未覆盖，保留报告等待复核")
    else:
        log("低频漂移审计完成；漂移只进入 staging/problem-drift.json，不自动覆盖发布内容")
    return result


def run_sentinel(args: argparse.Namespace) -> int:
    """Reverify a small rotating sample into an isolated evidence report."""

    if os.environ.get("YOJ_SYNC_ENABLE_SUBMIT") != "1":
        log("在线哨兵跳过：YOJ_SYNC_ENABLE_SUBMIT 不为 1；未登录、未提交、未改变发布状态")
        return 2
    selected, next_cursor = select_sentinel_problems(args.sentinel_count)
    if not selected:
        log("在线哨兵跳过：没有可轮换的 PUBLIC_READY 题目")
        return 0
    try:
        online_environment = prepare_online_environment()
    except RuntimeError as exc:
        log(f"在线哨兵未执行：{exc}")
        return 2
    command = [
        sys.executable,
        str(ROOT / "tools" / "online_verify.py"),
        "--report",
        str(ROOT / "staging" / "online-sentinel.json"),
        "--reverify-accepted",
        "--retry-abnormal",
        "--max-submissions",
        str(len(selected)),
        *problem_selector(selected),
    ]
    result = run_command(command, online_environment, 7200)
    remember_generated_changes()
    if result:
        log(
            f"在线哨兵未通过：题号 {selected} 的结果保留在 staging/online-sentinel.json；"
            "canonical 在线证据和 PUBLIC_READY 均未覆盖"
        )
        return result
    accepted, outcomes = sentinel_all_accepted(selected)
    if not accepted:
        log(
            f"在线哨兵发现非 Accepted/跳过结果：{outcomes}；"
            "不推进轮换游标，保留隔离报告等待复核"
        )
        return 3
    save_sentinel_state(selected, next_cursor)
    remember_generated_changes()
    log(f"在线哨兵完成：本轮轮换题号 {selected}；证据与发布状态隔离")
    return 0


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

        # A successful local ``git push`` is not the end of the release.  If
        # the previous cycle stopped before GitHub Actions/Pages reached a
        # terminal success, resolve that checkpoint before starting another
        # capture or publishing a second batch.
        if not verify_pending_remote():
            return 8

        environment = child_environment()
        if args.visibility_watch:
            return run_visibility_watch(args, environment)
        if args.drift_audit:
            return run_drift_audit(args, environment)
        if args.sentinel:
            return run_sentinel(args)
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
        snapshot_before = snapshot_content_digest(PUBLIC_SNAPSHOT_PATH)
        if run_command(
            [sys.executable, str(ROOT / "tools" / "audit_online_availability.py")],
            environment,
            600,
        ):
            log("公开题目列表快照未正常完成，本轮不继续抓取、构建或发布")
            return 3
        snapshot_changed = snapshot_content_digest(PUBLIC_SNAPSHOT_PATH) != snapshot_before

        if not run_visibility_audit(environment):
            log("公开题目可见性转变审计未正常完成，本轮不继续抓取、构建或发布")
            return 3
        visibility_report = load_visibility_report()
        if not visibility_report:
            log("调度暂停：公开题目可见性报告缺失或不可解析")
            remember_generated_changes()
            return 3

        # Discovery is intentionally unauthenticated.  It computes the same
        # local-history difference as the normal capture path, but does not
        # touch the account or Keychain until a genuinely new ID exists.
        discovery_command = [
            sys.executable,
            str(ROOT / "tools" / "yoj_capture.py"),
            "--discover-only",
            "--request-interval",
            str(args.request_interval),
            "--public-snapshot",
            str(PUBLIC_SNAPSHOT_PATH),
        ]
        if run_command(discovery_command, environment, 600):
            log("新题号发现阶段未正常完成，本轮不继续抓取、构建或发布")
            remember_generated_changes()
            return 3
        discovery_result = load_capture_result()
        if not discovery_result or discovery_result.get("scope") != "public_problem_numbers_only":
            log("调度暂停：新题号发现结果缺失或版本不受信，未继续后续阶段")
            remember_generated_changes()
            return 3
        try:
            discovered_problem_numbers = sorted(
                {int(value) for value in (discovery_result.get("newProblemNumbers") or [])}
            )
        except (TypeError, ValueError):
            log("调度暂停：新题号发现结果无法解析")
            remember_generated_changes()
            return 3

        backlog_problem_numbers = select_raw_cleanup_backlog()
        if backlog_problem_numbers:
            log(
                "本轮发现所有仍为 RAW_CAPTURED、已有归档 AC 代码且待清洗/在线复核的题目："
                f"{backlog_problem_numbers}"
            )

        if not discovered_problem_numbers and not backlog_problem_numbers:
            has_visibility_transition = bool(visibility_report.get("hasVisibilityTransitions"))
            if has_visibility_transition or snapshot_changed:
                archived = visibility_report.get("archivedProblemNumbers") or []
                reopened = visibility_report.get("reopenedProblemNumbers") or []
                if has_visibility_transition:
                    log(
                        "本轮没有新题号，但发现 YOJ 可见性转变："
                        f"下线 {archived}，重新开放 {reopened}；只刷新状态和快捷提交投影"
                    )
                else:
                    log("本轮没有新题号，但公开列表快照内容发生变化；刷新状态、README 和 Pages 目录")
                visibility_refresh = [
                    sys.executable,
                    str(ROOT / "tools" / "build_initial.py"),
                    "--visibility-only",
                ]
                if run_command(visibility_refresh, environment, 1800):
                    log("可见性轻量投影失败；保留旧基线，下一轮重试")
                    remember_generated_changes()
                    return 3
                for command in (
                    [sys.executable, str(ROOT / "tools" / "build_site_catalog.py"), "--check"],
                    [sys.executable, str(ROOT / "tools" / "audit_consistency.py")],
                ):
                    if run_command(command, environment, 600):
                        log("可见性轻量投影的离线一致性门禁失败；保留旧基线，下一轮重试")
                        remember_generated_changes()
                        return 3
                if args.publish:
                    result = publish(args.push, set())
                    if result:
                        log("可见性投影已生成但 Git 发布未完成；保留旧基线等待下一轮")
                        remember_generated_changes()
                        return result
            else:
                log("本轮无新题号、无可见性转变且公开列表快照未变化；跳过整库构建、编译、样例、在线复验和发布")
            if not run_visibility_audit(environment, commit=True):
                log("可见性基线保存失败；下一轮将重新核对")
                remember_generated_changes()
                return 3
            remember_generated_changes()
            return 0

        new_problem_numbers: list[int] = []
        if discovered_problem_numbers:
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
                str(PUBLIC_SNAPSHOT_PATH),
            ]
            if public_only:
                capture_command.append("--public-only")
            if run_command(capture_command, capture_environment, 3600):
                log("增量抓取未正常完成，本轮不继续构建、复验或发布")
                remember_generated_changes()
                return 3
            capture_result = load_capture_result()
            if not capture_result or capture_result.get("scope") != "public_problem_numbers_only":
                log("调度暂停：抓取结果缺失或版本不受信，未继续后续阶段")
                remember_generated_changes()
                return 3
            try:
                new_problem_numbers = sorted(
                    {int(value) for value in (capture_result.get("newProblemNumbers") or [])}
                )
            except (TypeError, ValueError):
                log("调度暂停：抓取结果中的新题号无法解析")
                remember_generated_changes()
                return 3
            if new_problem_numbers != discovered_problem_numbers:
                log(
                    "调度暂停：发现阶段与实际抓取阶段的新题号集合不一致；"
                    f"发现 {discovered_problem_numbers}，抓取 {new_problem_numbers}"
                )
                remember_generated_changes()
                return 3

        selected_problem_numbers = sorted(set(new_problem_numbers) | set(backlog_problem_numbers))
        selected = problem_selector(selected_problem_numbers)
        log(
            "本轮进入清洗/门禁/在线复核流水线："
            f"新题号 {new_problem_numbers}；待处理 backlog {backlog_problem_numbers}"
        )
        steps: list[tuple[list[str], dict[str, str], int]] = [
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
                        *(["--reverify-accepted", "--retry-abnormal"] if backlog_problem_numbers else []),
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
            # Keep selected new/backlog problem paths eligible for the scoped
            # publication allowlist.  The release gate still decides whether
            # code is actually materialized as PUBLIC_READY.
            release_numbers.update(selected_problem_numbers)
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
            if result == 0 and not run_visibility_audit(environment, commit=True):
                log("公开提交已完成，但可见性基线保存失败；下一轮会重试核对")
                remember_generated_changes()
                return 3
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
        if not run_visibility_audit(environment, commit=True):
            log("可见性基线保存失败；公开构建结果保留在本地待下一轮处理")
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
    parser.add_argument(
        "--visibility-watch",
        action="store_true",
        help="仅刷新公开列表并在变化时发布可见性投影；供每日主流程成功后的 launchd 间隔复查使用",
    )
    audit_mode = parser.add_mutually_exclusive_group()
    audit_mode.add_argument(
        "--drift-audit",
        action="store_true",
        help="低频读取当前公开题面，生成语义漂移复核队列；不构建、不提交、不发布",
    )
    audit_mode.add_argument(
        "--sentinel",
        action="store_true",
        help="低频轮换少量 PUBLIC_READY 题目在线复验，证据写入隔离报告",
    )
    parser.add_argument(
        "--drift-max-problems",
        type=int,
        default=int(os.environ.get("YOJ_DRIFT_MAX_PROBLEMS", "0")),
        help="漂移审计最多检查的题数；0 表示当前公开题目的全部题号",
    )
    parser.add_argument(
        "--drift-request-interval",
        type=float,
        default=float(os.environ.get("YOJ_DRIFT_REQUEST_INTERVAL", "1")),
        help="低频漂移审计的题面请求间隔秒数",
    )
    parser.add_argument(
        "--sentinel-count",
        type=int,
        default=int(os.environ.get("YOJ_SENTINEL_COUNT", "5")),
        help="在线哨兵每批轮换的 PUBLIC_READY 题目数，默认 5",
    )
    parser.add_argument("--max-submissions", type=int, default=int(os.environ.get("YOJ_MAX_SUBMISSIONS", "50")))
    parser.add_argument("--request-interval", type=float, default=float(os.environ.get("YOJ_REQUEST_INTERVAL", "1")))
    args = parser.parse_args()
    if (
        args.max_submissions < 0
        or args.request_interval < 0
        or args.drift_max_problems < 0
        or args.drift_request_interval < 0
        or args.sentinel_count < 0
    ):
        raise SystemExit("提交预算、题目数和请求间隔不能为负数")
    if args.dry_run and (args.allow_submit or args.publish or args.push or args.drift_audit or args.sentinel or args.visibility_watch):
        raise SystemExit("--dry-run 不能与在线提交或发布选项同时使用")
    if args.visibility_watch and (args.drift_audit or args.sentinel or args.allow_submit):
        raise SystemExit("--visibility-watch 仅允许公开列表投影，不能与提交或其他复核模式同时使用")
    if args.drift_audit and (args.allow_submit or args.publish or args.push):
        raise SystemExit("--drift-audit 只允许低频只读审计，不能附带提交或发布")
    if args.sentinel and (not args.allow_submit or args.publish or args.push):
        raise SystemExit("--sentinel 必须显式带 --allow-submit，且不能附带发布或推送")
    try:
        return run_once(args)
    except (OSError, RuntimeError, subprocess.SubprocessError) as exc:
        log(f"调度异常：{exc.__class__.__name__}: {exc}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
