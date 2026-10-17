"""Account discovery never silently falls back to the anonymous index."""
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools import yoj_capture as capture


class AccountDiscoveryTests(unittest.TestCase):
    def test_login_only_problem_is_discovered_without_scanning_submissions(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            events = []

            class Client:
                logged_in = False

                def login(self):
                    self.logged_in = True
                    events.append("login")

                def request(self, path):
                    events.append(path)
                    if '/course/' in path or '/contest/' in path:
                        return '<html><body><a href="/index.php/index/login/logout.html">logout</a><table></table></body></html>'
                    numbers = [1719, 1725] if self.logged_in else [1719]
                    return '<html><body>' + ''.join(f'<a href="/index.php/index/problem/detail/pno/{n}.html">p</a>' for n in numbers) + '</body></html>'

            with patch.object(capture, "YoJClient", Client), \
                 patch.object(capture, "existing_context", return_value=({}, {}, {1719})), \
                 patch.object(capture, "STATE_DIR", root), \
                 patch.object(capture, "CAPTURE_RESULT_PATH", root / "result.json"), \
                 patch.object(sys, "argv", ["capture", "--account-visible", "--discover-only", "--request-interval", "0"]):
                try:
                    result = capture.main()
                except SystemExit as exc:
                    result = exc.code
            self.assertEqual(result, 0)
            self.assertEqual(events[0], "login")
            self.assertFalse(any("submissions" in event for event in events))
            value = json.loads((root / "result.json").read_text())
            self.assertEqual(value["newProblemNumbers"], [1725])
            self.assertEqual(value["scope"], "account_visible_problem_numbers_only")
            snapshot = json.loads((root / "account-visible-problems.json").read_text())
            self.assertEqual(snapshot["problemNumbers"], [1719, 1725])
            self.assertEqual(snapshot['schemaVersion'], 2)
            self.assertEqual(snapshot['problemPaths']['1725'], ['/index.php/index/problem/detail/pno/1725.html'])

    def test_failed_login_cannot_reuse_anonymous_or_old_snapshot(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            snapshot = root / "account-visible-problems.json"
            snapshot.write_text('{"old":"evidence"}')

            class Client:
                def login(self):
                    raise RuntimeError("YOJ_AUTH_REQUIRED")

                def request(self, path):
                    raise AssertionError("list must not be read after failed login")

            with patch.object(capture, "YoJClient", Client), \
                 patch.object(capture, "existing_context", return_value=({}, {}, set())), \
                 patch.object(capture, "STATE_DIR", root), \
                 patch.object(capture, "CAPTURE_RESULT_PATH", root / "result.json"), \
                 patch.object(sys, "argv", ["capture", "--account-visible", "--discover-only"]):
                try:
                    capture.main()
                except SystemExit:
                    self.fail("account discovery flag is not implemented")
                except RuntimeError as exc:
                    self.assertEqual(str(exc), "YOJ_AUTH_REQUIRED")
                else:
                    self.fail("failed login was accepted")
            self.assertEqual(snapshot.read_text(), '{"old":"evidence"}')
            self.assertEqual(json.loads((root / "result.json").read_text())["scope"], "account_login_unavailable")


if __name__ == "__main__":
    unittest.main()
