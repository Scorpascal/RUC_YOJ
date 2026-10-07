#!/usr/bin/env python3
"""Read-only runner probes using the production receipt checker and TLS policy."""

from __future__ import annotations

import argparse
import base64
import contextlib
import io
import json
import os
from pathlib import Path
import ssl
import subprocess
import sys
import urllib.error
import urllib.request
from unittest.mock import patch

import yoj_health as health


REF_ENDPOINT = f"git/ref/heads/{health.BRANCH}"
RUNNERS = {"ubuntu-24.04", "ubuntu-26.04"}


def write_json(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def read_json(path: Path) -> dict:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise health.HealthError("INVALID_SNAPSHOT")
    return value


def ref_sha(ref: dict | None) -> str:
    if not isinstance(ref, dict) or ref.get("ref") != health.REF:
        raise health.HealthError("INVALID_REMOTE_REF")
    sha = ref.get("object", {}).get("sha", "")
    if not isinstance(sha, str) or not health.SHA_RE.fullmatch(sha):
        raise health.HealthError("INVALID_REMOTE_REF")
    return sha


def snapshot_cli(metadata: dict, ref: dict, contents: dict, summary_path: Path) -> dict:
    """Run original CLI at the selection time against exact downloaded responses."""
    cycle, sha = metadata["cycleDate"], ref_sha(ref)
    contents_endpoint = f"contents/receipts/{cycle}.json?ref={sha}"

    def fixed_response(endpoint: str) -> dict:
        if endpoint == REF_ENDPOINT:
            return ref
        if endpoint == contents_endpoint:
            return contents
        raise health.HealthError("UNEXPECTED_SNAPSHOT_REQUEST")

    now = health.timestamp(metadata["observedAt"])
    argv = ["yoj_health.py", "check", "--cycle-date", cycle, "--summary", str(summary_path)]
    with patch.object(health, "public_json", side_effect=fixed_response), \
            patch.object(health, "now_local", return_value=now), \
            patch.object(sys, "argv", argv), contextlib.redirect_stdout(io.StringIO()):
        results = health.check_cycles([cycle], now)
        exit_code = health.main()
    return {"exitCode": exit_code, "results": results,
            "summary": summary_path.read_text(encoding="utf-8")}


def select(args: argparse.Namespace) -> int:
    target = args.snapshot
    target.mkdir(parents=True, exist_ok=True)
    now = health.now_local()
    ref = health.public_json(REF_ENDPOINT)
    sha = ref_sha(ref)
    tree = health.public_json(f"git/trees/{sha}?recursive=1")
    if tree is None or tree.get("truncated") or not isinstance(tree.get("tree"), list):
        raise health.HealthError("INVALID_RECEIPT_TREE")
    cycles = sorted({match.group(1) for item in tree["tree"]
                     if isinstance(item, dict) and item.get("type") == "blob"
                     and (match := health.PATH_RE.fullmatch(item.get("path", "")))}, reverse=True)
    candidates = []
    for cycle in cycles[:7]:
        try:
            due = health.expected_cycles(now, health.ENABLED_FROM, cycle)
        except health.HealthError as exc:
            if str(exc) == "CYCLE_NOT_DUE":
                continue
            raise
        if not due:
            continue
        contents = health.public_json(f"contents/receipts/{cycle}.json?ref={sha}")
        if not isinstance(contents, dict):
            continue
        try:
            if (contents.get("type") != "file" or contents.get("encoding") != "base64"
                    or type(contents.get("size")) is not int
                    or not 0 <= contents["size"] <= health.MAX_BYTES):
                continue
            raw = base64.b64decode("".join(contents["content"].split()), validate=True)
            result = health.classify(health.decode(raw, cycle, now), cycle, now)
        except (health.HealthError, ValueError, KeyError, TypeError):
            continue
        if result["runState"] == "NO_TERMINAL_RECEIPT":
            continue
        rank = (result["scheduleState"] == "ON_TIME",
                result["runState"] == "RETURNED_OK", result["syncState"] == "SUCCEEDED")
        candidates.append((rank, cycle, contents))
        if all(rank):
            break
    if not candidates:
        raise health.HealthError("NO_COMPLETED_VALID_RECEIPT")
    _, cycle, contents = max(candidates, key=lambda item: (item[0], item[1]))
    metadata = {"schemaVersion": 1, "candidateSha": os.environ.get("GITHUB_SHA", "LOCAL"),
                "observedAt": health.stamp(now), "cycleDate": cycle, "receiptSha": sha,
                "receiptBlobSha": contents.get("sha")}
    metadata["expected"] = snapshot_cli(metadata, ref, contents, target / "expected-summary.md")
    write_json(target / "metadata.json", metadata)
    write_json(target / "ref.json", ref)
    write_json(target / "contents.json", contents)
    if output := os.environ.get("GITHUB_OUTPUT"):
        with open(output, "a", encoding="utf-8") as handle:
            handle.write(f"cycle_date={cycle}\nreceipt_sha={sha}\nreceipt_blob_sha={contents.get('sha', '')}\n")
    print(f"Selected cycle {cycle}, receipt branch {sha}, snapshot CLI exit {metadata['expected']['exitCode']}")
    return 0


def online_ref() -> dict:
    try:
        return {"status": "PASS", "sha": ref_sha(health.public_json(REF_ENDPOINT))}
    except (health.HealthError, OSError, ValueError, TypeError):
        return {"status": "FAIL", "error": "REF_OBSERVATION_UNAVAILABLE"}


def native_https(sha: str) -> dict:
    """No fallback: prove this Python's own verified HTTPS and trust roots work."""
    request = urllib.request.Request(
        f"https://api.github.com/repos/{health.REPOSITORY}/git/commits/{sha}",
        headers={"Accept": "application/vnd.github+json", "User-Agent": "YOJ-nightly-health",
                 "Cache-Control": "no-cache"})
    try:
        with urllib.request.urlopen(request, timeout=10) as response:
            raw = response.read(32769)
        value = json.loads(raw) if len(raw) <= 32768 else None
        if not isinstance(value, dict) or value.get("sha") != sha:
            return {"status": "FAIL", "error": "INVALID_HTTPS_RESPONSE"}
        return {"status": "PASS", "sha": sha}
    except urllib.error.HTTPError as exc:
        status = exc.code
        exc.close()
        category = "HTTP_RATE_LIMIT_OR_ACCESS_DENIED" if status in (403, 429) else "HTTP_ERROR"
        return {"status": "FAIL", "error": category, "httpStatus": status}
    except urllib.error.URLError as exc:
        reason = exc.reason
        category = "TLS_CERTIFICATE_ERROR" if isinstance(reason, ssl.SSLCertVerificationError) else "HTTPS_NETWORK_ERROR"
        return {"status": "FAIL", "error": category}
    except (OSError, ValueError, UnicodeError):
        return {"status": "FAIL", "error": "HTTPS_UNAVAILABLE"}


def curl_fallback(sha: str) -> dict:
    """Force only urllib's first failure; execute the existing real curl branch."""
    original_run = subprocess.run
    calls = []

    def tracked_run(command, *args, **kwargs):
        if command[0] == "curl":
            calls.append(command)
        return original_run(command, *args, **kwargs)

    try:
        with patch.object(health.urllib.request, "urlopen", side_effect=OSError("FORCED_TLS_PROBE")), \
                patch.object(health.subprocess, "run", side_effect=tracked_run):
            value = health.public_json(f"git/commits/{sha}")
        if not calls or not isinstance(value, dict) or value.get("sha") != sha:
            return {"status": "FAIL", "error": "INVALID_CURL_RESPONSE", "curlCalls": len(calls)}
        return {"status": "PASS", "sha": sha, "curlCalls": len(calls)}
    except (health.HealthError, OSError, subprocess.SubprocessError, ValueError):
        return {"status": "FAIL", "error": "CURL_FALLBACK_UNAVAILABLE", "curlCalls": len(calls)}


def online_cli(cycle: str, target: Path) -> dict:
    summary_path = target / "online-summary.md"
    try:
        process = subprocess.run(
            [sys.executable, "-B", str(Path(health.__file__).resolve()), "check",
             "--cycle-date", cycle, "--summary", str(summary_path)],
            capture_output=True, text=True, encoding="utf-8", timeout=100, check=False)
        summary = summary_path.read_text(encoding="utf-8") if summary_path.is_file() else ""
        states = {}
        for line in summary.splitlines():
            fields = [field.strip() for field in line.split("|")]
            if len(fields) == 9 and fields[1] == cycle:
                states = dict(zip(("scheduleState", "runState", "sourceState", "syncState", "reason",
                                   "lastSuccessfulSyncAt"), fields[2:8]))
        error = "OBSERVATION_ERROR" if "Check unavailable:" in summary else None
        return {"status": "PASS" if process.returncode == 0 and states else "FAIL",
                "exitCode": process.returncode, "states": states, "summary": summary, "error": error}
    except subprocess.TimeoutExpired:
        return {"status": "FAIL", "exitCode": None, "error": "ONLINE_CLI_TIMEOUT", "summary": "", "states": {}}
    except (OSError, ValueError, UnicodeError):
        return {"status": "FAIL", "exitCode": None, "error": "ONLINE_CLI_UNAVAILABLE", "summary": "", "states": {}}


def append_summary(text: str) -> None:
    if target := os.environ.get("GITHUB_STEP_SUMMARY"):
        with open(target, "a", encoding="utf-8") as handle:
            handle.write(text)


def observe(args: argparse.Namespace) -> int:
    target = args.output
    target.mkdir(parents=True, exist_ok=True)
    metadata = read_json(args.snapshot / "metadata.json")
    ref, contents = read_json(args.snapshot / "ref.json"), read_json(args.snapshot / "contents.json")
    sha = ref_sha(ref)
    if sha != metadata["receiptSha"] or contents.get("sha") != metadata["receiptBlobSha"]:
        raise health.HealthError("INVALID_SNAPSHOT")
    candidate_sha = os.environ.get("GITHUB_SHA", "LOCAL")
    if metadata["candidateSha"] != candidate_sha:
        raise health.HealthError("CANDIDATE_SHA_MISMATCH")
    before = online_ref()
    online = online_cli(metadata["cycleDate"], target)
    after = online_ref()
    # Each probe is recorded even if another failed. The final status is strict.
    native, fallback = native_https(sha), curl_fallback(sha)
    fixed = snapshot_cli(metadata, ref, contents, target / "snapshot-summary.md")
    fixed["status"] = "PASS" if fixed == metadata["expected"] else "FAIL"
    if before.get("sha") and after.get("sha"):
        ref_status = "STABLE" if before["sha"] == after["sha"] else "CHANGED"
    else:
        ref_status = "UNVERIFIED"
    passed = (fixed["status"] == "PASS" and online["status"] == "PASS"
              and native["status"] == "PASS" and fallback["status"] == "PASS"
              and ref_status != "UNVERIFIED")
    result = {"schemaVersion": 1, "runner": args.runner, "candidateSha": candidate_sha,
              "receiptSha": sha, "receiptBlobSha": metadata["receiptBlobSha"],
              "cycleDate": metadata["cycleDate"], "snapshotObservedAt": metadata["observedAt"],
              "onlineRefBefore": before, "onlineRefAfter": after, "onlineRefStatus": ref_status,
              "onlineCli": online, "snapshotCli": fixed, "nativeHttps": native,
              "curlFallback": fallback, "status": "PASS" if passed else "FAIL"}
    write_json(target / "result.json", result)
    report = (f"## Receipt probes: {args.runner}\n\nCandidate: `{candidate_sha}`; cycle: `{metadata['cycleDate']}`; "
              f"snapshot receipt branch: `{sha}`; blob: `{metadata['receiptBlobSha']}`.\n\n"
              f"Online branch before: `{before.get('sha', 'UNVERIFIED')}`; after: `{after.get('sha', 'UNVERIFIED')}` "
              f"({ref_status}).\n\n"
              "| Probe | Status | Exit/code |\n| --- | --- | --- |\n"
              f"| Original online check CLI | {online['status']} | {online['exitCode']} |\n"
              f"| Original CLI, fixed downloaded snapshot/time | {fixed['status']} | {fixed['exitCode']} |\n"
              f"| Python native verified HTTPS | {native['status']} | {native.get('error', 'OK')} |\n"
              f"| Existing real curl fallback | {fallback['status']} | {fallback.get('error', 'OK')} |\n\n"
              "Snapshot PASS means exact expected states, summary and exit code; a historical nonzero business exit remains visible. "
              "The snapshot does not prove online availability. Changed online refs do not invalidate the fixed snapshot.\n\n"
              f"### Original online summary\n\n{online['summary'] or 'No online summary produced.\n'}\n"
              f"### Fixed snapshot summary\n\n{fixed['summary']}\nOverall receipt/TLS probes: **{result['status']}**.\n")
    (target / "report.md").write_text(report, encoding="utf-8")
    append_summary(report)
    print(f"{args.runner}: snapshot={fixed['status']} (exit {fixed['exitCode']}), online={online['status']} "
          f"(exit {online['exitCode']}), native HTTPS={native['status']}, curl={fallback['status']}, ref={ref_status}")
    return int(not passed)


def compare(args: argparse.Namespace) -> int:
    results = [read_json(path) for path in sorted(args.results.rglob("result.json"))]
    found = {result.get("runner") for result in results}
    comparable = len(results) == 2 and found == RUNNERS
    identities = ("candidateSha", "receiptSha", "receiptBlobSha", "cycleDate", "snapshotObservedAt")
    if comparable:
        first, second = results
        comparable = all(first[key] == second[key] for key in identities)
    fixed_match = comparable and results[0]["snapshotCli"] == results[1]["snapshotCli"]
    online_shas = [result["onlineRefBefore"].get("sha") for result in results]
    online_comparable = (comparable and all(result["onlineRefStatus"] == "STABLE" for result in results)
                         and online_shas[0] == online_shas[1])
    online_match = (results[0]["onlineCli"] == results[1]["onlineCli"]) if online_comparable else None
    passed = (fixed_match and all(result["status"] == "PASS" for result in results)
              and online_match is not False)
    summary = {"status": "PASS" if passed else "FAIL", "runners": sorted(found),
               "sameCandidateAndSnapshot": comparable, "snapshotStatesSummaryAndExitMatch": fixed_match,
               "onlineComparable": online_comparable, "onlineStatesSummaryAndExitMatch": online_match,
               "results": results}
    args.output.mkdir(parents=True, exist_ok=True)
    write_json(args.output / "comparison.json", summary)
    lines = ["## Cross-runner receipt and HTTPS comparison", "",
             f"Overall: **{summary['status']}**; same candidate/snapshot: **{comparable}**; "
             f"fixed states/summary/exit match: **{fixed_match}**.", "",
             "| Runner | Snapshot exit/status | Online exit/status | schedule / run / sync | Native HTTPS | Real curl |",
             "| --- | --- | --- | --- | --- | --- |"]
    for result in results:
        fixed, online = result["snapshotCli"], result["onlineCli"]
        states = online["states"]
        lines.append(f"| {result['runner']} | {fixed['exitCode']} / {fixed['status']} | "
                     f"{online['exitCode']} / {online['status']} | {states.get('scheduleState', 'UNKNOWN')} / "
                     f"{states.get('runState', 'UNKNOWN')} / {states.get('syncState', 'UNKNOWN')} | "
                     f"{result['nativeHttps']['status']} | {result['curlFallback']['status']} |")
    lines.extend(["", f"Online branch snapshots directly comparable: **{online_comparable}**; "
                  f"online states/summary/exit match: **{online_match}**.", "",
                  "When live branch SHAs differ or move, online results are observations only. "
                  "The immutable downloaded responses and selection time provide the deterministic OS comparison.", ""])
    report = "\n".join(lines)
    (args.output / "comparison.md").write_text(report, encoding="utf-8")
    append_summary(report)
    print(report)
    return int(not passed)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    selection = commands.add_parser("select")
    selection.add_argument("--snapshot", type=Path, required=True)
    observation = commands.add_parser("observe")
    observation.add_argument("--snapshot", type=Path, required=True)
    observation.add_argument("--output", type=Path, required=True)
    observation.add_argument("--runner", choices=sorted(RUNNERS), required=True)
    comparison = commands.add_parser("compare")
    comparison.add_argument("--results", type=Path, required=True)
    comparison.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        return {"select": select, "observe": observe, "compare": compare}[args.command](args)
    except (health.HealthError, OSError, ValueError, KeyError, TypeError):
        print(f"::error::Receipt compatibility {args.command} unavailable; no verification claimed.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
