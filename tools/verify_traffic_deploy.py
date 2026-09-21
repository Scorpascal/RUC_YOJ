#!/usr/bin/env python3
"""Verify that the Pages deployment serves the exact traffic snapshot built."""

from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path
from urllib.parse import urlencode, urljoin, urlsplit, urlunsplit
from urllib.request import Request, urlopen

from build_traffic import validate_payload


def with_cache_buster(url: str, attempt: int) -> str:
    parts = urlsplit(url)
    query = urlencode({"traffic_verify": "1", "attempt": str(attempt)})
    return urlunsplit((parts.scheme, parts.netloc, parts.path, query, parts.fragment))


def fetch_json(url: str, attempt: int) -> object:
    request = Request(
        with_cache_buster(url, attempt),
        headers={"Cache-Control": "no-cache", "User-Agent": "RUC-YOJ-pages-traffic-verify/1.0"},
    )
    with urlopen(request, timeout=15) as response:
        return json.loads(response.read().decode("utf-8"))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url", required=True, help="deployed GitHub Pages URL")
    parser.add_argument("--expected", type=Path, required=True, help="local snapshot used for deployment")
    parser.add_argument("--attempts", type=int, default=8)
    args = parser.parse_args()

    try:
        expected = json.loads(args.expected.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        print(f"error: unable to read expected snapshot: {error}", file=sys.stderr)
        return 1
    errors = validate_payload(expected)
    if errors:
        print("error: expected snapshot is invalid: " + "; ".join(errors), file=sys.stderr)
        return 1

    base_url = args.url.rstrip("/") + "/"
    snapshot_url = urljoin(base_url, "data/traffic.json")
    expected_keys = ("schemaVersion", "source", "updatedAt", "today", "todayVisits", "totalVisits", "series")
    for attempt in range(1, max(1, args.attempts) + 1):
        try:
            actual = fetch_json(snapshot_url, attempt)
            actual_errors = validate_payload(actual)
            if actual_errors:
                raise ValueError("deployed snapshot is invalid: " + "; ".join(actual_errors))
            if any(actual.get(key) != expected.get(key) for key in expected_keys):
                raise ValueError("deployed snapshot does not match the built snapshot")
            print(f"deployed traffic snapshot verified: {snapshot_url}")
            return 0
        except Exception as error:  # pragma: no cover - depends on Pages propagation
            print(f"attempt {attempt}/{args.attempts}: {error}", file=sys.stderr)
            if attempt < args.attempts:
                time.sleep(5)

    print("error: Pages did not serve the expected traffic snapshot", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
