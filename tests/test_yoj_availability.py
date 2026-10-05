"""Offline public-index and per-attempt status regressions; no YOJ requests."""

import io
import json
import ssl
import tempfile
import unittest
from email.message import Message
from pathlib import Path
from unittest.mock import Mock, patch
from urllib.error import HTTPError, URLError
from urllib.response import addinfourl

from tools import audit_online_availability as audit, yoj_http as transport


def index(number=1, title="fixture", extra=""):
    return (f'<html><title>YOJ problems</title><table><tr><td><a href="/index.php/index/problem/detail/pno/{number}.html">'
            f'{title}</a></td></tr></table>{extra}</html>')


LOGIN = '<html><title>YOJ 登录</title><form action="/index.php/index/login/login2.html"><input type="password"></form></html>'
WEBVPN = '<html><title>中国人民大学WebVPN</title><form><input type="password"></form></html>'


def response(page, url=audit.PUBLIC_INDEX_URL):
    headers = Message()
    headers["Content-Type"] = "text/html; charset=utf-8"
    return addinfourl(io.BytesIO(page.encode("utf-8")), headers, url, 200)


class AvailabilityTests(unittest.TestCase):
    def run_audit(self, snapshot, status, responses, *extra_args):
        opener = Mock()
        opener.open.side_effect = responses
        args = ["audit", "--output", str(snapshot), "--status-output", str(status), *extra_args]
        with patch.object(audit, "build_yoj_opener", return_value=opener), \
             patch.object(transport.time, "sleep") as sleep, patch("sys.argv", args), \
             patch("sys.stdout", new_callable=io.StringIO) as stdout, \
             patch("sys.stderr", new_callable=io.StringIO) as stderr:
            result = audit.main()
        return result, json.loads(status.read_text()), opener, sleep, stdout.getvalue(), stderr.getvalue()

    def test_success_merges_advertised_pages_and_records_available(self):
        link = '<a href="/index.php/index/problem/index/p/2.html">next</a>'
        with tempfile.TemporaryDirectory() as directory:
            snapshot, status = Path(directory) / "snapshot.json", Path(directory) / "status.json"
            result, state, opener, sleep, _, stderr = self.run_audit(
                snapshot, status, [response(index(1, extra=link)), response(index(2))])
            self.assertEqual(result, 0)
            self.assertEqual(state["sourceState"], "AVAILABLE")
            self.assertEqual(state["reason"], "NONE")
            self.assertIs(state["snapshotChanged"], True)
            self.assertRegex(state["checkedAt"], r"^\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d\+08:00$")
            payload = json.loads(snapshot.read_text())
            self.assertEqual([row["problemNo"] for row in audit.validate_snapshot(payload)], [1, 2])
            self.assertEqual(payload["pagesFetched"], 2)
            self.assertEqual(opener.open.call_count, 2)
            sleep.assert_not_called()
            self.assertEqual(stderr, "")

    def test_unchanged_success_updates_status_not_snapshot_timestamp(self):
        with tempfile.TemporaryDirectory() as directory:
            snapshot, status = Path(directory) / "snapshot.json", Path(directory) / "status.json"
            with patch.object(audit, "checked_now", return_value="2026-10-03T22:30:00+08:00"):
                self.run_audit(snapshot, status, [response(index()), response(index())])
            before, modified = snapshot.read_bytes(), snapshot.stat().st_mtime_ns
            with patch.object(audit, "checked_now", return_value="2026-10-04T22:30:00+08:00"):
                result, state, _, _, stdout, _ = self.run_audit(snapshot, status, [response(index()), response(index())])
            self.assertEqual(result, 0)
            self.assertEqual(state["checkedAt"], "2026-10-04T22:30:00+08:00")
            self.assertEqual(state["sourceState"], "AVAILABLE")
            self.assertIs(state["snapshotChanged"], False)
            self.assertEqual(snapshot.read_bytes(), before)
            self.assertEqual(snapshot.stat().st_mtime_ns, modified)
            self.assertIn("unchanged", stdout)

    def test_legacy_pagination_still_stops_on_repeated_or_empty_page(self):
        for terminal in [index(2), "<html><title>YOJ problems</title><table></table></html>"]:
            with self.subTest(terminal=terminal), patch.object(audit, "fetch_page", side_effect=[index(1), index(2), terminal]):
                rows, count = audit.fetch_all_rows()
            self.assertEqual([row["problemNo"] for row in rows], [1, 2])
            self.assertEqual(count, 3)

    def test_all_blocked_first_requests_preserve_snapshot_without_retry_or_secret(self):
        cases = [
            (URLError(ssl.SSLCertVerificationError(1, "PRIVATE_MARKER")), "YOJ_TLS_CERTIFICATE_ERROR"),
            (response(WEBVPN), "YOJ_CAMPUS_ACCESS_REQUIRED"),
            (response(LOGIN), "YOJ_AUTH_REQUIRED"),
            (response(index(), "http://yoj.ruc.edu.cn/index.php/index/index/signin.html?private=PRIVATE_MARKER"), "YOJ_AUTH_REQUIRED"),
            (HTTPError(audit.PUBLIC_INDEX_URL, 401, "PRIVATE_MARKER", Message(), None), "YOJ_AUTH_REQUIRED"),
            (HTTPError(audit.PUBLIC_INDEX_URL, 403, "PRIVATE_MARKER", Message(), None), "YOJ_AUTH_REQUIRED"),
        ]
        for failed_request, reason in cases:
            with self.subTest(reason=reason), tempfile.TemporaryDirectory() as directory:
                snapshot, status = Path(directory) / "snapshot.json", Path(directory) / "status.json"
                snapshot.write_bytes(b"previous valid evidence")
                before, modified = snapshot.read_bytes(), snapshot.stat().st_mtime_ns
                result, state, opener, sleep, _, stderr = self.run_audit(snapshot, status, [failed_request])
                self.assertEqual(result, 1)
                self.assertEqual(state["sourceState"], "BLOCKED")
                self.assertEqual(state["reason"], reason)
                self.assertIsNone(state["snapshotChanged"])
                self.assertEqual(snapshot.read_bytes(), before)
                self.assertEqual(snapshot.stat().st_mtime_ns, modified)
                self.assertEqual(opener.open.call_count, 1)
                sleep.assert_not_called()
                self.assertNotIn("PRIVATE_MARKER", stderr + json.dumps(state))

    def test_failure_without_existing_snapshot_does_not_create_one(self):
        with tempfile.TemporaryDirectory() as directory:
            snapshot, status = Path(directory) / "snapshot.json", Path(directory) / "status.json"
            result, state, _, _, _, _ = self.run_audit(snapshot, status, [response(LOGIN)])
            self.assertEqual(result, 1)
            self.assertEqual(state["sourceState"], "BLOCKED")
            self.assertFalse(snapshot.exists())

    def test_login_or_webvpn_on_later_pages_cannot_publish_partial_index(self):
        link = '<a href="/index.php/index/problem/index/p/2.html">next</a>'
        for first in [index(1), index(1, extra=link)]:
            for portal, reason in [(LOGIN, "YOJ_AUTH_REQUIRED"), (WEBVPN, "YOJ_CAMPUS_ACCESS_REQUIRED"),
                                   (LOGIN.replace("</html>", index(99) + "</html>"), "YOJ_AUTH_REQUIRED")]:
                with self.subTest(advertised=bool(link in first), reason=reason), tempfile.TemporaryDirectory() as directory:
                    snapshot, status = Path(directory) / "snapshot.json", Path(directory) / "status.json"
                    audit.write_snapshot(audit.parse_rows(index(42)), snapshot)
                    before = snapshot.read_bytes()
                    result, state, opener, sleep, _, _ = self.run_audit(snapshot, status, [response(first), response(portal)])
                    self.assertEqual(result, 1)
                    self.assertEqual(state["reason"], reason)
                    self.assertEqual(snapshot.read_bytes(), before)
                    self.assertEqual(opener.open.call_count, 2)
                    sleep.assert_not_called()

    def test_login_and_vpn_navigation_in_a_valid_index_are_not_portals(self):
        page = index(1, title="login and WebVPN navigation", extra=(
            '<a href="/index.php/index/index/signin.html">登录</a>'
            '<a href="https://wvpn.ruc.edu.cn/help">WebVPN 帮助</a>'
            '<form action="/index.php/index/login/login2.html"><input type="password"></form>'))
        audit.validate_public_page(page)
        self.assertEqual(len(audit.parse_rows(page)), 1)

    def test_portal_structure_without_navigation_text_is_detected(self):
        page = '<title>Access</title><form action="https://wvpn.ruc.edu.cn/?private=PRIVATE_MARKER"><input type="text"></form>'
        with self.assertRaisesRegex(transport.YoJNetworkError, "YOJ_CAMPUS_ACCESS_REQUIRED"):
            audit.validate_public_page(page)
        with self.assertRaisesRegex(transport.YoJNetworkError, "YOJ_AUTH_REQUIRED"):
            audit.validate_public_page('<title>Sign in</title><p>Please authenticate</p>')

    def test_transient_failure_recovery_and_exhaustion(self):
        with tempfile.TemporaryDirectory() as directory:
            snapshot, status = Path(directory) / "snapshot.json", Path(directory) / "status.json"
            failures = [HTTPError(audit.PUBLIC_INDEX_URL, 429, "PRIVATE_MARKER", Message(), None),
                        HTTPError(audit.PUBLIC_INDEX_URL, 503, "PRIVATE_MARKER", Message(), None)]
            result, state, opener, sleep, _, _ = self.run_audit(
                snapshot, status, failures + [response(index()), response(index())])
            self.assertEqual(result, 0)
            self.assertEqual(state["sourceState"], "AVAILABLE")
            self.assertEqual(opener.open.call_count, 4)
            self.assertEqual(sleep.call_count, 2)
            before = snapshot.read_bytes()
            result, state, opener, sleep, _, stderr = self.run_audit(
                snapshot, status, [TimeoutError("PRIVATE_MARKER") for _ in range(3)])
            self.assertEqual(result, 1)
            self.assertEqual(state["sourceState"], "TRANSIENT_ERROR")
            self.assertEqual(state["reason"], "YOJ_TRANSIENT_NETWORK_ERROR")
            self.assertEqual(opener.open.call_count, 3)
            self.assertEqual(sleep.call_count, 2)
            self.assertEqual(snapshot.read_bytes(), before)
            self.assertNotIn("PRIVATE_MARKER", stderr)

    def test_invalid_index_or_unexpected_origin_is_error_not_empty_success(self):
        for page, reason in [("<html>maintenance</html>", "YOJ_INVALID_PUBLIC_INDEX"),
                             (index(1, title=""), "YOJ_INVALID_PUBLIC_INDEX"),
                             (index(1) + index(1, title="conflicting"), "YOJ_INVALID_PUBLIC_INDEX"),
                             (index(1).replace('href="/', 'href="https://example.invalid/'), "YOJ_UNEXPECTED_DESTINATION")]:
            with self.subTest(reason=reason), tempfile.TemporaryDirectory() as directory:
                snapshot, status = Path(directory) / "snapshot.json", Path(directory) / "status.json"
                snapshot.write_bytes(b"old evidence")
                result, state, _, sleep, _, _ = self.run_audit(snapshot, status, [response(page)])
                self.assertEqual(result, 1)
                self.assertEqual(state["sourceState"], "ERROR")
                self.assertEqual(state["reason"], reason)
                self.assertEqual(snapshot.read_bytes(), b"old evidence")
                sleep.assert_not_called()

    def test_unrelated_page_parameter_does_not_become_index_pagination(self):
        page = index(extra=(
            '<a href="/index.php/index/index/signin.html?page=2">登录</a>'
            '<a href="/index.php/index/user/index.html?p=3">用户</a>'
            '<a href="/index.php/index/problem/index.html?page=4">4</a>'))
        self.assertEqual(audit.public_page_links(page), {4: audit.PUBLIC_INDEX_URL + "?page=4"})

    def test_unknown_error_is_sanitized_and_snapshot_is_preserved(self):
        with tempfile.TemporaryDirectory() as directory:
            snapshot, status = Path(directory) / "snapshot.json", Path(directory) / "status.json"
            snapshot.write_bytes(b"old evidence")
            result, state, _, _, _, stderr = self.run_audit(snapshot, status, [RuntimeError("PRIVATE_MARKER")])
            self.assertEqual(result, 1)
            self.assertEqual(state["reason"], "YOJ_SOURCE_ERROR")
            self.assertEqual(snapshot.read_bytes(), b"old evidence")
            self.assertNotIn("PRIVATE_MARKER", stderr + json.dumps(state))

    def test_atomic_snapshot_failure_preserves_previous_file_and_removes_temp(self):
        with tempfile.TemporaryDirectory() as directory:
            snapshot = Path(directory) / "snapshot.json"
            audit.write_snapshot(audit.parse_rows(index(42)), snapshot)
            before = snapshot.read_bytes()
            with patch.object(Path, "replace", side_effect=OSError("fixture write failure")), self.assertRaises(OSError):
                audit.write_snapshot(audit.parse_rows(index(1)), snapshot)
            self.assertEqual(snapshot.read_bytes(), before)
            self.assertEqual(list(Path(directory).iterdir()), [snapshot])
            with patch.object(Path, "replace", side_effect=OSError("PRIVATE_MARKER")), \
                 patch.object(audit, "fetch_all_rows", return_value=(audit.parse_rows(index(1)), 1)), \
                 patch("sys.argv", ["audit", "--output", str(snapshot)]), \
                 patch("sys.stdout", new_callable=io.StringIO), patch("sys.stderr", new_callable=io.StringIO) as stderr:
                self.assertEqual(audit.main(), 1)
            self.assertEqual(snapshot.read_bytes(), before)
            self.assertNotIn("PRIVATE_MARKER", stderr.getvalue())
            self.assertEqual(list(Path(directory).iterdir()), [snapshot])

    def test_old_check_cli_still_validates_without_network_or_status(self):
        with tempfile.TemporaryDirectory() as directory:
            snapshot = Path(directory) / "snapshot.json"
            audit.write_snapshot(audit.parse_rows(index()), snapshot)
            with patch("sys.argv", ["audit", "--check", "--output", str(snapshot)]), \
                 patch.object(audit, "build_yoj_opener") as opener, patch("sys.stdout", new_callable=io.StringIO):
                self.assertEqual(audit.main(), 0)
            opener.assert_not_called()

    def test_status_cannot_overwrite_snapshot_or_claim_offline_check_is_live(self):
        with tempfile.TemporaryDirectory() as directory:
            snapshot = Path(directory) / "snapshot.json"
            snapshot.write_bytes(b"old evidence")
            for extra in [[], ["--check"]]:
                with patch("sys.argv", ["audit", "--output", str(snapshot), "--status-output", str(snapshot), *extra]), \
                     patch("sys.stderr", new_callable=io.StringIO), self.assertRaises(SystemExit) as raised:
                    audit.main()
                self.assertEqual(raised.exception.code, 2)
                self.assertEqual(snapshot.read_bytes(), b"old evidence")

    def test_status_write_failure_warns_without_changing_source_result(self):
        for source_error, expected_result in [(None, 0), (transport.YoJNetworkError("YOJ_AUTH_REQUIRED"), 1)]:
            with self.subTest(expected_result=expected_result), tempfile.TemporaryDirectory() as directory:
                snapshot, status = Path(directory) / "snapshot.json", Path(directory) / "status.json"
                audit.write_snapshot(audit.parse_rows(index(42)), snapshot)
                before = snapshot.read_bytes()
                rows = audit.parse_rows(index(1))
                with patch("sys.argv", ["audit", "--output", str(snapshot), "--status-output", str(status)]), \
                     patch.object(audit, "fetch_all_rows", return_value=(rows, 1), side_effect=source_error), \
                     patch.object(audit, "write_status", side_effect=PermissionError("PRIVATE_MARKER")), \
                     patch("sys.stdout", new_callable=io.StringIO), patch("sys.stderr", new_callable=io.StringIO) as stderr:
                    self.assertEqual(audit.main(), expected_result)
                self.assertIn("online availability status warning: YOJ_SOURCE_ERROR", stderr.getvalue())
                self.assertNotIn("PRIVATE_MARKER", stderr.getvalue())
                if source_error is None:
                    self.assertEqual(audit.validate_snapshot(json.loads(snapshot.read_text())), rows)
                else:
                    self.assertEqual(snapshot.read_bytes(), before)
                self.assertFalse(status.exists())

    def test_input_html_cannot_generate_live_source_status(self):
        with tempfile.TemporaryDirectory() as directory:
            source, snapshot, status = (Path(directory) / name for name in ("input.html", "snapshot.json", "status.json"))
            source.write_text(index(), encoding="utf-8")
            snapshot.write_bytes(b"old evidence")
            with patch("sys.argv", ["audit", "--input-html", str(source), "--output", str(snapshot), "--status-output", str(status)]), \
                 patch.object(audit, "build_yoj_opener") as opener, patch("sys.stderr", new_callable=io.StringIO), \
                 self.assertRaises(SystemExit) as raised:
                audit.main()
            self.assertEqual(raised.exception.code, 2)
            opener.assert_not_called()
            self.assertEqual(snapshot.read_bytes(), b"old evidence")
            self.assertFalse(status.exists())


if __name__ == "__main__":
    unittest.main()
