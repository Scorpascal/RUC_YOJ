"""Offline checks for campus redirects and verified TLS, without any account."""

import io
import os
import ssl
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from email.message import Message
from unittest.mock import Mock, patch
from urllib.request import HTTPHandler, Request
from urllib.response import addinfourl

from tools import audit_online_availability as audit, online_verify, yoj_http as transport


class TransportTests(unittest.TestCase):
    def test_actual_opener_stops_at_campus_redirect_without_second_request(self):
        visited = []
        responses = []

        class CampusGateway(HTTPHandler):
            def http_open(self, request):
                visited.append(request.full_url)
                headers = Message()
                headers["Location"] = "http://wvpn.ruc.edu.cn/login?private=PRIVATE_MARKER"
                response = addinfourl(io.BytesIO(b"redirect"), headers, request.full_url, 302)
                response.msg = "Found"
                responses.append(response)
                return response

        opener = transport.build_yoj_opener(CampusGateway())
        with self.assertRaisesRegex(transport.YoJNetworkError, "YOJ_CAMPUS_ACCESS_REQUIRED"):
            opener.open("http://yoj.ruc.edu.cn/index.php/index/problem/index.html")
        self.assertEqual(len(visited), 1)
        self.assertNotIn("PRIVATE_MARKER", visited[0])
        self.assertTrue(responses[0].closed)

    def test_campus_and_foreign_redirects_are_blocked_before_forwarding(self):
        request = Request("http://yoj.ruc.edu.cn/index.php/index/problem/index.html",
                          headers={"Cookie": "PRIVATE_MARKER"})
        handler = transport.YoJRedirectHandler()
        for destination, error in [
            ("https://wvpn.ruc.edu.cn/login/index?redirect_authurl=PRIVATE_MARKER", "YOJ_CAMPUS_ACCESS_REQUIRED"),
            ("https://example.invalid/?token=PRIVATE_MARKER", "YOJ_UNEXPECTED_DESTINATION"),
            ("https://yoj.ruc.edu.cn.example.invalid/", "YOJ_UNEXPECTED_DESTINATION"),
        ]:
            with self.subTest(destination=destination), self.assertRaisesRegex(transport.YoJNetworkError, error) as raised:
                handler.redirect_request(request, None, 302, "Found", {}, destination)
            self.assertNotIn("PRIVATE_MARKER", str(raised.exception))

    def test_same_origin_redirect_works_but_https_downgrade_does_not(self):
        handler = transport.YoJRedirectHandler()
        source = "http://yoj.ruc.edu.cn/index.php/problem/index"
        target = "https://yoj.ruc.edu.cn/index.php/index/problem/index.html"
        redirected = handler.redirect_request(Request(source), None, 302, "Found", {}, target)
        self.assertEqual(redirected.full_url, target)
        with self.assertRaisesRegex(transport.YoJNetworkError, "DOWNGRADE"):
            handler.redirect_request(Request(target), None, 302, "Found", {}, source)
        for url in ("file:///etc/passwd", "http://user:pass@yoj.ruc.edu.cn/", "http://yoj.ruc.edu.cn:8080/"):
            with self.assertRaises(transport.YoJNetworkError):
                transport.validate_yoj_url(url)

    def test_missing_framework_roots_use_system_bundle_not_insecure_tls(self):
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        with patch.object(transport.ssl, "create_default_context", return_value=context), \
             patch.object(transport.ssl, "get_default_verify_paths", return_value=SimpleNamespace(cafile=None)), \
             patch.object(transport.sys, "platform", "darwin"), patch.dict(os.environ, {}, clear=True), \
             patch.object(Path, "is_file", return_value=True), patch.object(context, "load_verify_locations") as load:
            self.assertIs(transport.verified_context(), context)
        load.assert_called_once_with(cafile="/etc/ssl/cert.pem")
        self.assertTrue(context.check_hostname)
        self.assertEqual(context.verify_mode, ssl.CERT_REQUIRED)

    def test_explicit_trust_configuration_and_linux_are_not_overridden(self):
        for platform, env, cafile in [("linux", {}, None), ("darwin", {"SSL_CERT_FILE": "custom.pem"}, None),
                                     ("darwin", {"SSL_CERT_DIR": "custom-roots"}, None), ("darwin", {}, "working.pem")]:
            context = Mock()
            with patch.object(transport.ssl, "create_default_context", return_value=context), \
                 patch.object(transport.ssl, "get_default_verify_paths", return_value=SimpleNamespace(cafile=cafile)), \
                 patch.object(transport.sys, "platform", platform), patch.dict(os.environ, env, clear=True):
                transport.verified_context()
            context.load_verify_locations.assert_not_called()

    def test_campus_error_leaves_snapshot_intact_and_does_not_echo_private_url(self):
        with tempfile.TemporaryDirectory() as directory:
            snapshot = Path(directory) / "snapshot.json"
            snapshot.write_bytes(b"previous valid evidence")
            opener = Mock()
            opener.open.side_effect = transport.YoJNetworkError("YOJ_CAMPUS_ACCESS_REQUIRED")
            with patch.object(audit, "build_yoj_opener", return_value=opener), \
                 patch("sys.argv", ["audit", "--output", str(snapshot)]), patch("sys.stderr", new_callable=io.StringIO) as output:
                self.assertEqual(audit.main(), 1)
            self.assertEqual(snapshot.read_bytes(), b"previous valid evidence")
            self.assertIn("YOJ_CAMPUS_ACCESS_REQUIRED", output.getvalue())

    def test_authenticated_client_shares_guard_and_does_not_retry_submissions(self):
        opener = Mock()
        opener.open.side_effect = transport.YoJNetworkError("YOJ_CAMPUS_ACCESS_REQUIRED")
        with patch.object(online_verify, "build_yoj_opener", return_value=opener), \
             patch.object(online_verify.time, "sleep") as sleep:
            client = online_verify.YoJClient()
            with self.assertRaises(transport.YoJNetworkError):
                client.request("/index.php/index/index/prob_submit.html", {"code": "fixture"})
            self.assertEqual(opener.open.call_count, 1)
            sleep.assert_not_called()
            opener.reset_mock()
            with self.assertRaises(transport.YoJNetworkError):
                client.request("https://example.invalid/", {"code": "fixture"})
            opener.open.assert_not_called()


if __name__ == "__main__":
    unittest.main()
