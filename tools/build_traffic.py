#!/usr/bin/env python3
"""Refresh the public traffic snapshot used by the GitHub Pages home page.

Visitor Badge is intentionally queried with ``query_only=true`` here.  The
browser page owns the incrementing probes; this job only reads the total and
seven date-scoped counters so the static Pages site can render real numbers
without needing a server-side API.
"""

from __future__ import annotations

import argparse
import json
import re
import ssl
import sys
import time
from datetime import date, datetime, timedelta, timezone
from pathlib import Path
from urllib.parse import urlencode
from urllib.request import Request, urlopen
from zoneinfo import ZoneInfo

try:
    import certifi
except ImportError:  # pragma: no cover - GitHub-hosted Python has system CAs
    certifi = None


ROOT = Path(__file__).resolve().parents[1]
OUTPUT_PATH = ROOT / "docs" / "data" / "traffic.json"
BADGE_ENDPOINT = "https://visitor-badge.laobi.icu/badge"
PAGE_ID = "Scorpascal.RUC_YOJ"
TIMEZONE_NAME = "Asia/Shanghai"
LOCAL_ZONE = ZoneInfo(TIMEZONE_NAME)
FETCH_ATTEMPTS = 3
FETCH_TIMEOUT_SECONDS = 15


def badge_url(page_id: str) -> str:
    return f"{BADGE_ENDPOINT}?{urlencode({'page_id': page_id, 'query_only': 'true'})}"


def parse_count(svg: str) -> int | None:
    """Extract the final numeric text node from Visitor Badge's SVG."""

    # Visitor Badge uses ``Error`` for a date-scoped counter that has not been
    # created yet; for this dashboard that is a legitimate zero.  Its explicit
    # ``Count API Failed`` response is different and must remain incomplete so
    # a provider outage cannot overwrite a valid snapshot.
    if re.search(r">[^<]*Count API Failed[^<]*<", svg, flags=re.IGNORECASE):
        return None
    if re.search(r">[^<]*Error[^<]*<", svg, flags=re.IGNORECASE):
        return 0
    values = re.findall(r">([0-9][0-9,]*)<", svg)
    numbers = [int(value.replace(",", "")) for value in values]
    return numbers[-1] if numbers else None


def fetch_count(page_id: str, attempts: int = FETCH_ATTEMPTS) -> int | None:
    request = Request(
        badge_url(page_id),
        headers={"User-Agent": "RUC-YOJ-pages-traffic/1.0"},
    )
    context = ssl.create_default_context(cafile=certifi.where()) if certifi else ssl.create_default_context()
    last_error: Exception | None = None
    for attempt in range(1, attempts + 1):
        try:
            with urlopen(request, timeout=FETCH_TIMEOUT_SECONDS, context=context) as response:
                body = response.read().decode("utf-8", errors="replace")
            count = parse_count(body)
            if count is None:
                raise ValueError("Visitor Badge returned an error or no numeric count")
            return count
        except Exception as error:  # pragma: no cover - depends on remote service
            last_error = error
            if attempt < attempts:
                time.sleep(min(attempt * 2, 4))
    print(
        f"warning: unable to read {page_id} after {attempts} attempts: {last_error}",
        file=sys.stderr,
    )
    return None


def date_label(day: date) -> str:
    return f"{day.month}/{day.day}"


def validate_payload(payload: object) -> list[str]:
    """Return invariant violations for a published traffic snapshot."""

    errors: list[str] = []
    if not isinstance(payload, dict):
        return ["payload must be a JSON object"]
    if payload.get("schemaVersion") != 1:
        errors.append("schemaVersion must be 1")

    source = payload.get("source")
    if not isinstance(source, dict):
        errors.append("source must be an object")
    else:
        expected_source = {
            "provider": "visitor-badge",
            "pageId": PAGE_ID,
            "timezone": TIMEZONE_NAME,
            "countSemantics": "page_loads",
        }
        for key, expected in expected_source.items():
            if source.get(key) != expected:
                errors.append(f"source.{key} must be {expected!r}")

    today = payload.get("today")
    try:
        today_date = date.fromisoformat(today) if isinstance(today, str) else None
    except ValueError:
        today_date = None
    if today_date is None:
        errors.append("today must be an ISO date")

    series = payload.get("series")
    if not isinstance(series, list) or len(series) != 7:
        errors.append("series must contain exactly seven entries")
        series = series if isinstance(series, list) else []

    series_dates: list[date] = []
    for index, item in enumerate(series):
        if not isinstance(item, dict):
            errors.append(f"series[{index}] must be an object")
            continue
        item_date = item.get("date")
        try:
            parsed_date = date.fromisoformat(item_date) if isinstance(item_date, str) else None
        except ValueError:
            parsed_date = None
        if parsed_date is None:
            errors.append(f"series[{index}].date must be an ISO date")
        else:
            series_dates.append(parsed_date)
        visits = item.get("visits")
        if isinstance(visits, bool) or not isinstance(visits, int) or visits < 0:
            errors.append(f"series[{index}].visits must be a non-negative integer")

    if len(series_dates) == 7:
        expected_dates = [series_dates[0] + timedelta(days=index) for index in range(7)]
        if series_dates != expected_dates:
            errors.append("series dates must be contiguous and ascending")
        if today_date is not None and series_dates[-1] != today_date:
            errors.append("series[-1].date must equal today")

    today_visits = payload.get("todayVisits")
    total_visits = payload.get("totalVisits")
    if isinstance(today_visits, bool) or not isinstance(today_visits, int) or today_visits < 0:
        errors.append("todayVisits must be a non-negative integer")
    if isinstance(total_visits, bool) or not isinstance(total_visits, int) or total_visits < 0:
        errors.append("totalVisits must be a non-negative integer")
    if series and isinstance(series[-1], dict) and today_visits != series[-1].get("visits"):
        errors.append("todayVisits must equal series[-1].visits")
    if isinstance(today_visits, int) and isinstance(total_visits, int) and total_visits < today_visits:
        errors.append("totalVisits must be at least todayVisits")

    updated_at = payload.get("updatedAt")
    if not isinstance(updated_at, str):
        errors.append("updatedAt must be an ISO timestamp")
    else:
        try:
            parsed_updated_at = datetime.fromisoformat(updated_at.replace("Z", "+00:00"))
            if parsed_updated_at.tzinfo is None:
                errors.append("updatedAt must include a timezone")
        except ValueError:
            errors.append("updatedAt must be an ISO timestamp")

    return errors


def build_payload(today: date | None = None) -> dict:
    now_local = datetime.now(LOCAL_ZONE)
    current_day = today or now_local.date()
    series: list[dict] = []
    for offset in range(6, -1, -1):
        day = current_day - timedelta(days=offset)
        # Keep this identical to the browser probe in docs/index.html.
        day_id = f"{PAGE_ID}.day.{day:%Y-%m-%d}"
        series.append(
            {
                "date": day.isoformat(),
                "label": date_label(day),
                "visits": fetch_count(day_id),
            }
        )

    payload = {
        "schemaVersion": 1,
        "source": {
            "provider": "visitor-badge",
            "pageId": PAGE_ID,
            "timezone": TIMEZONE_NAME,
            "countSemantics": "page_loads",
        },
        "updatedAt": datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z"),
        "today": current_day.isoformat(),
        "todayVisits": series[-1]["visits"],
        "totalVisits": fetch_count(PAGE_ID),
        "series": series,
    }
    errors = validate_payload(payload)
    if errors:
        raise ValueError("; ".join(errors))
    return payload


def check_existing_snapshot() -> int:
    try:
        payload = json.loads(OUTPUT_PATH.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        print(f"error: unable to read traffic snapshot: {error}", file=sys.stderr)
        return 1
    errors = validate_payload(payload)
    if errors:
        print("error: invalid traffic snapshot:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1
    print(f"traffic snapshot valid: {OUTPUT_PATH}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="只校验已有快照，不访问统计服务")
    args = parser.parse_args()
    if args.check:
        return check_existing_snapshot()

    try:
        payload = build_payload()
    except ValueError as error:
        print(f"error: traffic snapshot is incomplete; preserving the previous file: {error}", file=sys.stderr)
        return 1
    errors = validate_payload(payload)
    if errors:
        print("error: traffic snapshot is incomplete; preserving the previous file", file=sys.stderr)
        return 1

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"traffic snapshot written: {OUTPUT_PATH}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
