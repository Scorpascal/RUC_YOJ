#!/usr/bin/env python3
"""Refresh the public traffic snapshot used by the GitHub Pages home page.

Visitor Badge is intentionally queried with ``query_only=true`` here.  The
browser page owns the incrementing probes; this job only reads the total and
seven date-scoped counters so the static Pages site can render real numbers
without needing a server-side API.
"""

from __future__ import annotations

import json
import re
import ssl
import sys
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


def badge_url(page_id: str) -> str:
    return f"{BADGE_ENDPOINT}?{urlencode({'page_id': page_id, 'query_only': 'true'})}"


def parse_count(svg: str) -> int | None:
    """Extract the final numeric text node from Visitor Badge's SVG."""

    if ">Error<" in svg:
        return 0
    values = re.findall(r">([0-9][0-9,]*)<", svg)
    numbers = [int(value.replace(",", "")) for value in values]
    return numbers[-1] if numbers else None


def fetch_count(page_id: str) -> int | None:
    request = Request(
        badge_url(page_id),
        headers={"User-Agent": "RUC-YOJ-pages-traffic/1.0"},
    )
    try:
        context = ssl.create_default_context(cafile=certifi.where()) if certifi else ssl.create_default_context()
        with urlopen(request, timeout=20, context=context) as response:
            body = response.read().decode("utf-8", errors="replace")
    except Exception as error:  # pragma: no cover - depends on remote service
        print(f"warning: unable to read {page_id}: {error}", file=sys.stderr)
        return None
    return parse_count(body)


def date_label(day: date) -> str:
    return f"{day.month}/{day.day}"


def build_payload(today: date | None = None) -> dict:
    current_day = today or datetime.now(LOCAL_ZONE).date()
    series: list[dict] = []
    for offset in range(6, -1, -1):
        day = current_day - timedelta(days=offset)
        day_id = f"{PAGE_ID}.day.{day:%Y%m%d}"
        series.append(
            {
                "date": day.isoformat(),
                "label": date_label(day),
                "visits": fetch_count(day_id),
            }
        )

    return {
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


def main() -> int:
    payload = build_payload()
    if payload["totalVisits"] is None or any(item["visits"] is None for item in payload["series"]):
        print("error: traffic snapshot is incomplete; preserving the previous file", file=sys.stderr)
        return 1

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"traffic snapshot written: {OUTPUT_PATH}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
