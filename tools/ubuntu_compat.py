#!/usr/bin/env python3
"""Read-only diagnostics for the isolated Ubuntu runner compatibility workflow."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import ssl
import subprocess
import sys
import tarfile
from zoneinfo import ZoneInfo
from datetime import datetime


def inventory(output: Path) -> None:
    versions = {}
    for command in ("git", "curl", "tar"):
        result = subprocess.run([command, "--version"], capture_output=True,
                                text=True, check=True, timeout=10)
        versions[command] = result.stdout.splitlines()[0]
    result = {
        "candidate_sha": subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip(),
        "image_os": os.environ.get("ImageOS", "not supplied"),
        "image_version": os.environ.get("ImageVersion", "not supplied"),
        "os_release": Path("/etc/os-release").read_text(encoding="utf-8"),
        "kernel": platform.release(),
        "python_path": sys.executable,
        "python_version": platform.python_version(),
        "python_openssl": ssl.OPENSSL_VERSION,
        "python_ca_paths": ssl.get_default_verify_paths()._asdict(),
        "tzdata_package": subprocess.check_output(
            ["dpkg-query", "--show", "--showformat=${Version}", "tzdata"],
            text=True, timeout=10),
        "filesystem_encoding": sys.getfilesystemencoding(),
        "shanghai": datetime(2026, 10, 8, tzinfo=ZoneInfo("Asia/Shanghai")).isoformat(),
        "tools": versions,
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(output.read_text(encoding="utf-8"))
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with Path(summary).open("a", encoding="utf-8") as handle:
            handle.write("## Runner inventory\n\n```json\n" + output.read_text(encoding="utf-8") + "```\n")


def verify_archive(archive: Path, root: Path, output: Path) -> None:
    # Match upload-pages-artifact's default exclusion of hidden path components.
    expected = {path.relative_to(root).as_posix(): path for path in root.rglob("*")
                if path.is_file() and not any(part.startswith(".") for part in path.relative_to(root).parts)}
    required = {"index.html", "yoj-quick-submit.html", "data/catalog.json",
                "data/quick-submit.json", "data/traffic.json"}
    if not required <= expected.keys():
        raise ValueError("Required site files are absent from docs")
    actual = {}
    subprocess.run(["tar", "-tf", str(archive)], capture_output=True, check=True, timeout=30)
    with tarfile.open(archive, "r:") as handle:
        for member in handle:
            name = member.name.removeprefix("./")
            if member.isdir():
                continue
            if not member.isfile() or name in actual or name not in expected:
                raise ValueError(f"Unexpected or duplicate archive entry: {name}")
            source = handle.extractfile(member)
            if source is None:
                raise ValueError(f"Unreadable archive entry: {name}")
            digest = hashlib.sha256(source.read()).hexdigest()
            if digest != hashlib.sha256(expected[name].read_bytes()).hexdigest():
                raise ValueError(f"Archive content differs: {name}")
            actual[name] = digest
    if actual.keys() != expected.keys():
        raise ValueError("Archive file manifest differs from docs")
    result = {"status": "passed", "files": len(actual), "required": sorted(required),
              "sha256": actual}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Verified downloaded Pages artifact: {len(actual)} files; all SHA-256 hashes match docs")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    inspect = commands.add_parser("inventory")
    inspect.add_argument("--output", type=Path, required=True)
    archive = commands.add_parser("archive")
    archive.add_argument("--archive", type=Path, required=True)
    archive.add_argument("--root", type=Path, default=Path("docs"))
    archive.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.command == "inventory":
        inventory(args.output)
    else:
        verify_archive(args.archive, args.root, args.output)


if __name__ == "__main__":
    main()
