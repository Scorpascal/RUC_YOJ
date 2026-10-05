"""Keep a locally committed, gated-off push out of nightly success evidence."""

from __future__ import annotations

import tempfile
import unittest
from contextlib import ExitStack
from datetime import datetime
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from tools import yoj_scheduler as scheduler, yoj_sync_status as status


class PublicationStatusTests(unittest.TestCase):
    def test_real_publication_with_push_gate_closed_preserves_previous_success(self):
        with tempfile.TemporaryDirectory() as directory, ExitStack() as stack:
            root = Path(directory)
            state = root / ".yoj-sync"
            state.mkdir()
            ready = root / "data" / "public-ready.json"
            ready.parent.mkdir()
            ready.write_text('{"records": []}\n', encoding="utf-8")
            terms = root / "method" / "personal_terms.txt"
            terms.parent.mkdir()
            terms.write_text("", encoding="utf-8")
            catalog = root / "docs" / "data" / "catalog.json"
            catalog.parent.mkdir(parents=True)
            catalog.write_text('{"records": []}\n', encoding="utf-8")

            now = datetime.fromisoformat("2026-10-05T22:30:00+08:00")
            base_sha = "a" * 40
            head_sha = base_sha
            changed = False
            staged: list[str] = []
            git_commands: list[list[str]] = []
            publish_results: list[int] = []
            published_path = "docs/data/catalog.json"
            args = SimpleNamespace(
                allow_submit=True, publish=True, push=True, dry_run=False,
                visibility_watch=False, drift_audit=False, sentinel=False,
                request_interval=0, max_submissions=50,
            )

            def git(command, **_kwargs):
                nonlocal head_sha, changed
                git_commands.append(list(command))
                if command == ["git", "rev-parse", "origin/main"]:
                    output = base_sha + "\n"
                elif command == ["git", "rev-parse", "HEAD"]:
                    output = head_sha + "\n"
                elif command == ["git", "status", "--porcelain=v1", "--untracked-files=all", "-z"]:
                    output = (" M " + published_path + "\0").encode() if changed else b""
                elif command == ["git", "diff", "--cached", "--name-only", "-z"]:
                    output = b"".join(path.encode() + b"\0" for path in staged)
                elif command == ["git", "diff", "--check", "--", published_path]:
                    output = ""
                elif command == ["git", "add", "--", published_path]:
                    staged.append(published_path)
                    output = ""
                elif command == ["git", "commit", "-m", "chore: sync YOJ archive 2026-10-05 22:45"]:
                    self.assertEqual(staged, [published_path])
                    staged.clear()
                    changed = False
                    head_sha = "b" * 40
                    output = "fixture local commit\n"
                else:
                    self.fail(f"Unexpected subprocess command: {command!r}")
                return SimpleNamespace(returncode=0, stdout=output, stderr="")

            def command(command, *_args, **_kwargs):
                tool = Path(command[1]).name
                permitted = {
                    "audit_online_availability.py", "yoj_capture.py", "build_initial.py",
                    "build_site_catalog.py", "audit_consistency.py",
                }
                self.assertIn(tool, permitted)
                self.assertEqual(command[1], str(root / "tools" / tool))
                if tool == "yoj_capture.py":
                    self.assertIn("--discover-only", command)
                if tool == "build_initial.py":
                    self.assertEqual(command[2:], ["--visibility-only"])
                if tool == "build_site_catalog.py":
                    self.assertEqual(command[2:], ["--check"])
                return 0

            real_publish = scheduler.publish

            def publish(*args, **kwargs):
                self.assertTrue(args[0])
                result = real_publish(*args, **kwargs)
                publish_results.append(result)
                return result

            for name, value in {
                "ROOT": root, "STATE_DIR": state, "LOCK_PATH": state / "scheduler.lock",
                "PENDING_REMOTE_PATH": state / "pending.json", "PUBLIC_READY_PATH": ready,
                "PUBLIC_SNAPSHOT_PATH": root / "public.json",
            }.items():
                stack.enter_context(patch.object(scheduler, name, value))
            for name, value in {
                "in_maintenance": False, "validate_worktree_before_run": True,
                "eligible_unpublished_numbers": [], "child_environment": {},
                "snapshot_content_digest": "same", "run_visibility_audit": True,
                "load_capture_result": {"scope": "public_problem_numbers_only", "newProblemNumbers": []},
                "select_pending_personal_ac_targets": [], "select_raw_cleanup_backlog": [],
                "remember_generated_changes": None, "log": None,
            }.items():
                stack.enter_context(patch.object(scheduler, name, return_value=value))
            visibility = stack.enter_context(patch.object(
                scheduler, "load_visibility_report", return_value={"hasVisibilityTransitions": False},
            ))
            final_audit = stack.enter_context(patch.object(
                scheduler, "run_staged_consistency_audit", return_value=True,
            ))
            stack.enter_context(patch.object(scheduler, "now_local", side_effect=lambda: now))
            stack.enter_context(patch.object(scheduler, "run_command", side_effect=command))
            stack.enter_context(patch.object(scheduler, "publish", side_effect=publish))
            stack.enter_context(patch.object(
                scheduler, "prepare_online_environment", side_effect=AssertionError("Unexpected login"),
            ))
            stack.enter_context(patch.object(scheduler.subprocess, "run", side_effect=git))
            for name in ("Popen", "call", "check_call", "check_output"):
                stack.enter_context(patch.object(
                    scheduler.subprocess, name, side_effect=AssertionError("Unexpected subprocess"),
                ))
            stack.enter_context(patch.dict(scheduler.os.environ, {
                "YOJ_SYNC_ALLOW_PUSH": "0", "YOJ_SYNC_ENABLE_SUBMIT": "1", "YOJ_HEALTH_CONTEXT": "",
            }))

            # A valid unchanged cycle creates independent successful evidence.
            self.assertEqual(scheduler.run_once(args), 0)
            previous = status.read(state)
            self.assertIsNotNone(previous)
            self.assertEqual(previous["syncState"], "SUCCEEDED")
            last_success = previous["lastSuccessfulSyncAt"]
            self.assertEqual(last_success, now.isoformat())
            self.assertEqual(publish_results, [])

            # Use the real publisher when visibility projection needs release.
            now = datetime.fromisoformat("2026-10-05T22:45:00+08:00")
            changed = True
            visibility.return_value = {"hasVisibilityTransitions": True}
            self.assertEqual(scheduler.run_once(args), 0)
            current = status.read(state)
            self.assertIsNotNone(current)
            self.assertEqual(publish_results, [0])
            self.assertEqual(head_sha, "b" * 40)
            self.assertEqual(current["sourceState"], "AVAILABLE")
            self.assertEqual(current["syncState"], "INCOMPLETE")
            self.assertEqual(current["reason"], "SYNC_INCOMPLETE")
            self.assertEqual(current["lastSuccessfulSyncAt"], last_success)
            final_audit.assert_called_once()
            scheduler.run_visibility_audit.assert_called_with({}, commit=True)
            self.assertEqual(len([cmd for cmd in git_commands if cmd[:2] == ["git", "commit"]]), 1)
            self.assertFalse(any(cmd[:2] == ["git", "push"] for cmd in git_commands))
            self.assertFalse((state / "pending.json").exists())


if __name__ == "__main__":
    unittest.main()
