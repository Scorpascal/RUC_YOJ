"""Offline checks for campus redirects and verified TLS, without any account."""

import io
import os
import socket
import ssl
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from email.message import Message
from unittest.mock import Mock, patch
from urllib.request import HTTPHandler, Request
from urllib.error import HTTPError, URLError
from urllib.response import addinfourl

from tools import audit_online_availability as audit, online_verify, yoj_http as transport


class TransportTests(unittest.TestCase):
    def response(self, body=b"public fixture", url=audit.PUBLIC_INDEX_URL):
        return addinfourl(io.BytesIO(body), Message(), url, 200)

    def test_error_categories_keep_private_diagnostics_out_of_safe_error(self):
        cases = [
            (ssl.SSLCertVerificationError(1, "PRIVATE_MARKER"), "YOJ_TLS_CERTIFICATE_ERROR"),
            (URLError(ssl.SSLCertVerificationError(1, "PRIVATE_MARKER")), "YOJ_TLS_CERTIFICATE_ERROR"),
            (TimeoutError("PRIVATE_MARKER"), "YOJ_TRANSIENT_NETWORK_ERROR"),
            (URLError(ConnectionResetError("PRIVATE_MARKER")), "YOJ_TRANSIENT_NETWORK_ERROR"),
            (URLError(socket.gaierror(socket.EAI_AGAIN, "PRIVATE_MARKER")), "YOJ_TRANSIENT_NETWORK_ERROR"),
            (URLError(socket.gaierror(socket.EAI_NONAME, "PRIVATE_MARKER")), "YOJ_SOURCE_ERROR"),
            (ssl.SSLError(1, "PRIVATE_MARKER"), "YOJ_SOURCE_ERROR"),
            (URLError("PRIVATE_MARKER"), "YOJ_SOURCE_ERROR"),
            (ValueError("PRIVATE_MARKER"), "YOJ_SOURCE_ERROR"),
        ]
        for error, reason in cases:
            with self.subTest(error=type(error).__name__, reason=reason):
                self.assertEqual(transport.classify_yoj_error(error), reason)
                self.assertEqual(str(transport.YoJNetworkError(reason + ": PRIVATE_MARKER")), reason)
        self.assertEqual(str(transport.YoJNetworkError("PRIVATE_MARKER")), "YOJ_SOURCE_ERROR")

    def test_http_status_categories(self):
        for code, reason in [(401, "YOJ_AUTH_REQUIRED"), (403, "YOJ_AUTH_REQUIRED"),
                             (429, "YOJ_TRANSIENT_NETWORK_ERROR"), (500, "YOJ_TRANSIENT_NETWORK_ERROR"),
                             (503, "YOJ_TRANSIENT_NETWORK_ERROR"), (404, "YOJ_SOURCE_ERROR")]:
            with self.subTest(code=code):
                error = HTTPError("https://yoj.ruc.edu.cn/?secret=PRIVATE_MARKER", code,
                                  "PRIVATE_MARKER", Message(), None)
                try:
                    self.assertEqual(transport.classify_yoj_error(error), reason)
                finally:
                    error.close()

    def test_public_get_retries_only_temporary_failure_and_succeeds_on_third(self):
        opener = Mock()
        opener.open.side_effect = [TimeoutError("PRIVATE_MARKER"),
                                   URLError(socket.gaierror(socket.EAI_AGAIN, "PRIVATE_MARKER")),
                                   self.response()]
        with patch.object(transport.time, "sleep") as sleep:
            body, charset, destination = transport.read_public_get(opener, Request(audit.PUBLIC_INDEX_URL), 30)
        self.assertEqual((body, charset, destination), (b"public fixture", "utf-8", audit.PUBLIC_INDEX_URL))
        self.assertEqual(opener.open.call_count, 3)
        self.assertEqual([call.args[0] for call in sleep.call_args_list], [2, 4])

    def test_public_get_exhaustion_has_three_attempts_and_no_private_error(self):
        opener = Mock()
        opener.open.side_effect = TimeoutError("https://yoj.ruc.edu.cn/?secret=PRIVATE_MARKER")
        with patch.object(transport.time, "sleep") as sleep, \
             self.assertRaises(transport.YoJNetworkError) as raised:
            transport.read_public_get(opener, Request(audit.PUBLIC_INDEX_URL), 30)
        self.assertEqual(str(raised.exception), "YOJ_TRANSIENT_NETWORK_ERROR")
        self.assertEqual(opener.open.call_count, 3)
        self.assertEqual(sleep.call_count, 2)

    def test_public_get_does_not_retry_tls_access_or_unknown_error(self):
        for error in [URLError(ssl.SSLCertVerificationError(1, "PRIVATE_MARKER")),
                      transport.YoJNetworkError("YOJ_CAMPUS_ACCESS_REQUIRED"),
                      transport.YoJNetworkError("YOJ_UNEXPECTED_DESTINATION"),
                      HTTPError(audit.PUBLIC_INDEX_URL, 401, "PRIVATE_MARKER", Message(), None),
                      HTTPError(audit.PUBLIC_INDEX_URL, 403, "PRIVATE_MARKER", Message(), None),
                      URLError(socket.gaierror(socket.EAI_NONAME, "PRIVATE_MARKER")),
                      URLError("PRIVATE_MARKER")]:
            opener = Mock()
            opener.open.side_effect = error
            with self.subTest(error=type(error).__name__), patch.object(transport.time, "sleep") as sleep, \
                 self.assertRaises(transport.YoJNetworkError) as raised:
                transport.read_public_get(opener, Request(audit.PUBLIC_INDEX_URL), 30)
            self.assertEqual(opener.open.call_count, 1)
            sleep.assert_not_called()
            self.assertNotIn("PRIVATE_MARKER", str(raised.exception))

    def test_public_get_retries_read_timeout_and_closes_each_response(self):
        first = self.response()
        second = self.response()
        opener = Mock()
        opener.open.side_effect = [first, second]
        with patch.object(first, "read", side_effect=TimeoutError("PRIVATE_MARKER")), \
             patch.object(transport.time, "sleep"):
            self.assertEqual(transport.read_public_get(opener, Request(audit.PUBLIC_INDEX_URL), 30)[0], b"public fixture")
        self.assertTrue(first.closed)
        self.assertTrue(second.closed)
        self.assertEqual(opener.open.call_count, 2)

    def test_public_retry_helper_rejects_post_before_opening(self):
        opener = Mock()
        with self.assertRaises(ValueError):
            transport.read_public_get(opener, Request(audit.PUBLIC_INDEX_URL, data=b"fixture"), 30)
        opener.open.assert_not_called()

    def test_final_response_destination_is_still_checked(self):
        for url, reason in [("https://wvpn.ruc.edu.cn/?secret=PRIVATE_MARKER", "YOJ_CAMPUS_ACCESS_REQUIRED"),
                            ("https://example.invalid/?secret=PRIVATE_MARKER", "YOJ_UNEXPECTED_DESTINATION"),
                            (audit.PUBLIC_INDEX_URL, "YOJ_HTTPS_DOWNGRADE_REFUSED")]:
            response = self.response(url=url)
            opener = Mock()
            opener.open.return_value = response
            request = Request(audit.PUBLIC_INDEX_URL.replace("http:", "https:"))
            with self.subTest(reason=reason), self.assertRaisesRegex(transport.YoJNetworkError, reason):
                transport.read_public_get(opener, request, 30)
            self.assertTrue(response.closed)
            self.assertEqual(opener.open.call_count, 1)

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
