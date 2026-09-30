"""Offline regression checks for nightly personal-AC refresh."""

from __future__ import annotations

import json
import hashlib
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from tools import yoj_capture, yoj_scheduler, online_verify


def write_json(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False), encoding="utf-8")


class CaptureRefreshTests(unittest.TestCase):
    def test_incomplete_personal_submission_pages_fail_closed(self) -> None:
        first_page = (
            '<tr><td><a href="/index.php/index/submissions/detail/subno/2001.html">提交</a>'
            '<a href="/index.php/index/problem/detail/pno/1717.html">题目</a>'
            '<span class="status">Accepted</span></td></tr>'
            '<a href="/index.php/submissions/index/p/2.html">2</a>'
        )
        with patch.object(yoj_capture, "fetch", side_effect=[first_page, "<html>临时错误</html>"]):
            with self.assertRaisesRegex(RuntimeError, "提前中断"):
                yoj_capture.all_submission_rows(object(), object(), 250, require_complete=True)

    def test_existing_topic_becomes_raw_ac_once_without_touching_published_problem(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            manifest = root / "代码库" / "AC抓取清单.json"
            original_ready = {
                "problemNo": "42", "title": "已发布", "folder": "0042_已发布",
                "submissionNo": "900", "files": {"completeCode": "0042_已发布/0042_代码.cpp"},
            }
            topic = {
                "problemNo": "1717", "title": "进程通信", "folder": "1717_原目录",
                "submissionNo": "", "status": "TOPIC_CAPTURED",
                "files": {"problem": "1717_原目录/1717_题目原文.html", "metadata": "1717_原目录/1717_元数据.json"},
            }
            write_json(manifest, {"problems": [original_ready, topic], "totals": {}})
            write_json(root / "data" / "problems.json", {"records": [{"problemNo": "42"}, {"problemNo": "1717"}]})
            write_json(root / "data" / "public-ready.json", {"records": [{"problemNo": 42, "status": "PUBLIC_READY"}]})
            code_requests: list[str] = []

            class FakeClient:
                def login(self) -> None:
                    pass

                def request(self, path: str, data: dict | None = None, referer: str | None = None) -> str:
                    code_requests.append(path)
                    return json.dumps({"status": "1", "code": "int main(){return 0;}\n"})

            rows = [{"problemNo": 1717, "submissionNo": 2001, "status": "Accepted", "language": "cpp17"}]
            state = root / ".yoj-sync"
            patches = [
                patch.object(yoj_capture, "ROOT", root),
                patch.object(yoj_capture, "MANIFEST_PATH", manifest),
                patch.object(yoj_capture, "DATA_PROBLEMS_PATH", root / "data" / "problems.json"),
                patch.object(yoj_capture, "PUBLIC_READY_PATH", root / "data" / "public-ready.json"),
                patch.object(yoj_capture, "LOCAL_PROBLEM_ROOTS", (root / "代码库", root / "题解")),
                patch.object(yoj_capture, "STATE_DIR", state),
                patch.object(yoj_capture, "CAPTURE_RESULT_PATH", state / "capture-result.json"),
                patch.object(yoj_capture, "YoJClient", FakeClient),
                patch.object(yoj_capture, "load_public_snapshot", return_value=({42, 1717}, 0)),
                patch.object(yoj_capture, "all_submission_rows", return_value=rows),
                patch.object(yoj_capture, "fetch", return_value="<h1>进程通信</h1>"),
                patch.object(sys, "argv", ["yoj_capture.py", "--public-snapshot", "unused", "--ac-target", "1717", "--request-interval", "0"]),
            ]
            with patches[0], patches[1], patches[2], patches[3], patches[4], patches[5], patches[6], patches[7], patches[8], patches[9], patches[10], patches[11]:
                self.assertEqual(yoj_capture.main(), 0)
                refreshed = json.loads(manifest.read_text(encoding="utf-8"))
                by_no = {row["problemNo"]: row for row in refreshed["problems"]}
                self.assertEqual(len(refreshed["problems"]), 2)
                self.assertEqual(by_no["42"], original_ready)
                self.assertEqual(by_no["1717"]["submissionNo"], "2001")
                self.assertEqual(by_no["1717"]["folder"], "1717_原目录")
                self.assertTrue((root / "代码库" / by_no["1717"]["files"]["completeCode"]).is_file())
                self.assertEqual(json.loads((state / "capture-result.json").read_text())["refreshedAcNumbers"], [1717])
                self.assertEqual(yoj_capture.main(), 0)
                self.assertEqual(json.loads((state / "capture-result.json").read_text())["refreshedAcNumbers"], [])
                self.assertEqual(code_requests.count("/index.php/index/submissions/getcode.html"), 1)


class SchedulerRefreshTests(unittest.TestCase):
    def test_publication_saves_exact_commit_before_first_push(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory)
            base_sha, commit_sha = "a" * 40, "b" * 40
            staged: list[str] = []

            def git(command: list[str], **_kwargs: object) -> SimpleNamespace:
                if command[:2] == ["git", "add"]:
                    staged.extend(command[3:])
                if command[:3] == ["git", "rev-parse", "origin/main"]:
                    return SimpleNamespace(returncode=0, stdout=base_sha + "\n", stderr="")
                if command[:3] == ["git", "branch", "--show-current"]:
                    return SimpleNamespace(returncode=0, stdout="main\n", stderr="")
                if command[:2] == ["git", "push"]:
                    self.assertEqual(yoj_scheduler.load_pending_remote()["state"], "push-pending")
                    self.assertEqual(yoj_scheduler.load_pending_remote()["commit"], commit_sha)
                    return SimpleNamespace(returncode=128, stdout="", stderr="network unavailable")
                return SimpleNamespace(returncode=0, stdout="", stderr="")

            with (
                patch.object(yoj_scheduler, "STATE_DIR", state),
                patch.object(yoj_scheduler, "PENDING_REMOTE_PATH", state / "pending.json"),
                patch.object(yoj_scheduler, "staged_paths", side_effect=lambda: list(staged)),
                patch.object(yoj_scheduler, "changed_paths", return_value=["data/public-ready.json"]),
                patch.object(yoj_scheduler, "pending_unreleased_code_paths", return_value=[]),
                patch.object(yoj_scheduler, "scan_for_secrets", return_value=None),
                patch.object(yoj_scheduler, "scan_for_personal_markers", return_value=None),
                patch.object(yoj_scheduler, "run_staged_consistency_audit", return_value=True),
                patch.object(yoj_scheduler, "git_head_sha", side_effect=[base_sha, commit_sha, commit_sha]),
                patch.object(yoj_scheduler.subprocess, "run", side_effect=git),
                patch.object(yoj_scheduler, "log"),
                patch.dict(yoj_scheduler.os.environ, {"YOJ_SYNC_ALLOW_PUSH": "1"}),
            ):
                self.assertEqual(yoj_scheduler.publish(True, {42}), 8)
                self.assertEqual(yoj_scheduler.load_pending_remote()["state"], "push-failed")

    def test_failed_push_is_checkpointed_then_only_same_commit_retried(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory)
            commit_sha = "b" * 40
            push_attempts: list[list[str]] = []

            def git(command: list[str], **_kwargs: object) -> SimpleNamespace:
                if command[:3] == ["git", "branch", "--show-current"]:
                    return SimpleNamespace(returncode=0, stdout="main\n", stderr="")
                if command[:2] == ["git", "merge-base"]:
                    return SimpleNamespace(returncode=0, stdout="", stderr="")
                if command[:2] == ["git", "push"]:
                    push_attempts.append(command)
                    return SimpleNamespace(returncode=128 if len(push_attempts) == 1 else 0,
                                           stdout="", stderr="temporary network failure")
                raise AssertionError(command)

            with (
                patch.object(yoj_scheduler, "STATE_DIR", state),
                patch.object(yoj_scheduler, "PENDING_REMOTE_PATH", state / "pending.json"),
                patch.object(yoj_scheduler, "git_head_sha", return_value=commit_sha),
                patch.object(yoj_scheduler.subprocess, "run", side_effect=git),
                patch.object(yoj_scheduler, "verify_remote_workflow", return_value=True) as workflow,
                patch.object(yoj_scheduler, "log"),
                patch.dict(yoj_scheduler.os.environ, {"YOJ_SYNC_ALLOW_PUSH": "1"}),
            ):
                yoj_scheduler.save_pending_remote(commit_sha, state="push-pending")
                self.assertFalse(yoj_scheduler.verify_pending_remote(allow_push=True))
                self.assertEqual(yoj_scheduler.load_pending_remote()["state"], "push-failed")
                workflow.assert_not_called()
                self.assertFalse(yoj_scheduler.verify_pending_remote(allow_push=False))
                self.assertEqual(len(push_attempts), 1)
                self.assertTrue(yoj_scheduler.verify_pending_remote(allow_push=True))
                self.assertEqual(push_attempts, [["git", "push", "origin", "main"]] * 2)
                workflow.assert_called_once_with(commit_sha)
                self.assertIsNone(yoj_scheduler.load_pending_remote())

    def test_changed_head_cannot_expand_retry_scope(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory)
            with (
                patch.object(yoj_scheduler, "STATE_DIR", state),
                patch.object(yoj_scheduler, "PENDING_REMOTE_PATH", state / "pending.json"),
                patch.object(yoj_scheduler, "git_head_sha", return_value="c" * 40),
                patch.object(yoj_scheduler.subprocess, "run") as git,
                patch.object(yoj_scheduler, "log"),
                patch.dict(yoj_scheduler.os.environ, {"YOJ_SYNC_ALLOW_PUSH": "1"}),
            ):
                yoj_scheduler.save_pending_remote("b" * 40, state="push-pending")
                git.return_value = SimpleNamespace(returncode=0, stdout="main\n", stderr="")
                self.assertFalse(yoj_scheduler.verify_pending_remote(allow_push=True))
                self.assertEqual(yoj_scheduler.load_pending_remote()["state"], "push-failed")
                self.assertFalse(any(call.args[0][:2] == ["git", "push"] for call in git.call_args_list))

    def test_pushed_checkpoint_waits_for_matching_pages_without_repush(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory)
            with (
                patch.object(yoj_scheduler, "STATE_DIR", state),
                patch.object(yoj_scheduler, "PENDING_REMOTE_PATH", state / "pending.json"),
                patch.object(yoj_scheduler, "verify_remote_workflow", side_effect=[False, True]) as workflow,
                patch.object(yoj_scheduler.subprocess, "run") as git,
                patch.object(yoj_scheduler, "log"),
            ):
                yoj_scheduler.save_pending_remote("b" * 40, state="deployment-pending")
                self.assertFalse(yoj_scheduler.verify_pending_remote())
                self.assertTrue(yoj_scheduler.verify_pending_remote())
                self.assertEqual(workflow.call_count, 2)
                git.assert_not_called()
                self.assertIsNone(yoj_scheduler.load_pending_remote())

    def test_released_folder_does_not_publish_raw_siblings(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            manifest = Path(directory) / "public-ready.json"
            clean = "代码库/1717_题目/1717_clean.cpp"
            raw = "代码库/1717_题目/1717_raw.cpp"
            write_json(manifest, {"records": [{"problemNo": 1717, "completeCode": clean, "directlySubmittableCode": clean}]})
            with patch.object(yoj_scheduler, "PUBLIC_READY_PATH", manifest):
                self.assertTrue(yoj_scheduler.path_is_publishable(clean, {1717}))
                self.assertFalse(yoj_scheduler.path_is_publishable(raw, {1717}))

    def test_pending_raw_does_not_block_other_verified_publication(self) -> None:
        raw = "代码库/1717_原目录/1717_代码.cpp"
        staged: list[str] = []
        def git(command: list[str], **_kwargs: object) -> SimpleNamespace:
            if command[:2] == ["git", "add"]:
                staged.extend(command[3:])
            return SimpleNamespace(returncode=0, stdout="", stderr="")
        with (
            patch.object(yoj_scheduler, "staged_paths", side_effect=lambda: list(staged)),
            patch.object(yoj_scheduler, "changed_paths", return_value=[raw, "data/public-ready.json"]),
            patch.object(yoj_scheduler, "public_ready_problem_numbers", return_value={42}),
            patch.object(yoj_scheduler, "scan_for_personal_markers", return_value=None),
            patch.object(yoj_scheduler, "run_staged_consistency_audit", return_value=True),
            patch.object(yoj_scheduler, "log"),
            patch.object(yoj_scheduler.subprocess, "run", side_effect=git),
        ):
            self.assertEqual(yoj_scheduler.publish(False, {42}), 0)
        self.assertEqual(staged, ["data/public-ready.json"])

    def test_interrupted_online_batch_publishes_passed_rows_but_remains_retryable(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory)
            args = SimpleNamespace(visibility_watch=False, drift_audit=False, sentinel=False, dry_run=False,
                                   request_interval=0, allow_submit=True, max_submissions=50, publish=True, push=False)
            results = iter([
                {"scope": "public_problem_numbers_only", "newProblemNumbers": []},
                {"scope": "public_problem_numbers_and_personal_accepted", "newProblemNumbers": [], "newTopicNumbers": [], "acTargetNumbers": [1717], "refreshedAcNumbers": [1717], "submissionScan": "FULL_TARGETED"},
            ])
            with (
                patch.object(yoj_scheduler, "STATE_DIR", state),
                patch.object(yoj_scheduler, "LOCK_PATH", state / "lock"),
                patch.object(yoj_scheduler, "in_maintenance", return_value=False),
                patch.object(yoj_scheduler, "validate_worktree_before_run", return_value=True),
                patch.object(yoj_scheduler, "verify_pending_remote", return_value=True),
                patch.object(yoj_scheduler, "child_environment", return_value={}),
                patch.object(yoj_scheduler, "prepare_online_environment", return_value={}),
                patch.object(yoj_scheduler, "snapshot_content_digest", return_value="same"),
                patch.object(yoj_scheduler, "run_visibility_audit", return_value=True),
                patch.object(yoj_scheduler, "load_visibility_report", return_value={"hasVisibilityTransitions": False}),
                patch.object(yoj_scheduler, "load_capture_result", side_effect=lambda: next(results)),
                patch.object(yoj_scheduler, "select_pending_personal_ac_targets", return_value=[1717]),
                patch.object(yoj_scheduler, "select_raw_cleanup_backlog", return_value=[]),
                patch.object(yoj_scheduler, "eligible_unpublished_numbers", return_value=[]),
                patch.object(yoj_scheduler, "publish_verified_checkpoint", return_value=0) as release,
                patch.object(yoj_scheduler, "run_command", side_effect=lambda cmd, *_a: 3 if "online_verify.py" in " ".join(cmd) else 0),
                patch.object(yoj_scheduler, "remember_generated_changes"),
                patch.object(yoj_scheduler, "log"),
                patch.dict(yoj_scheduler.os.environ, {"YOJ_SYNC_ENABLE_SUBMIT": "1"}),
            ):
                self.assertEqual(yoj_scheduler.run_once(args), 3)
                release.assert_called_once()

    def test_saved_release_precedes_failed_public_list_request(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory)
            events: list[str] = []
            args = SimpleNamespace(visibility_watch=False, drift_audit=False, sentinel=False, dry_run=False,
                                   publish=True, push=False)
            with (
                patch.object(yoj_scheduler, "STATE_DIR", state),
                patch.object(yoj_scheduler, "LOCK_PATH", state / "lock"),
                patch.object(yoj_scheduler, "in_maintenance", return_value=False),
                patch.object(yoj_scheduler, "validate_worktree_before_run", return_value=True),
                patch.object(yoj_scheduler, "verify_pending_remote", return_value=True),
                patch.object(yoj_scheduler, "child_environment", return_value={}),
                patch.object(yoj_scheduler, "snapshot_content_digest", return_value="same"),
                patch.object(yoj_scheduler, "eligible_unpublished_numbers", return_value=[1703]),
                patch.object(yoj_scheduler, "publish_verified_checkpoint", side_effect=lambda *_a: events.append("release") or 0),
                patch.object(yoj_scheduler, "run_command", side_effect=lambda *_a: events.append("network") or 1),
                patch.object(yoj_scheduler, "log"),
            ):
                self.assertEqual(yoj_scheduler.run_once(args), 3)
            self.assertEqual(events, ["release", "network"])

    def test_publisher_waits_when_new_raw_source_has_not_passed_release_gate(self) -> None:
        raw_path = "代码库/1717_原目录/1717_代码.cpp"
        with (
            patch.object(yoj_scheduler, "staged_paths", return_value=[]),
            patch.object(yoj_scheduler, "changed_paths", return_value=[raw_path]),
            patch.object(yoj_scheduler, "public_ready_problem_numbers", return_value=set()),
            patch.object(yoj_scheduler, "log"),
            patch.object(yoj_scheduler.subprocess, "run") as git_run,
        ):
            self.assertEqual(yoj_scheduler.publish(True, {1717}), 0)
            git_run.assert_not_called()

    def test_unreleased_raw_code_cannot_enter_public_commit(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            terms = root / "method" / "personal_terms.txt"
            terms.parent.mkdir(parents=True)
            terms.write_text("private_id\n", encoding="utf-8")
            raw_path = "代码库/1717_原目录/1717_代码.cpp"
            code = root / raw_path
            code.parent.mkdir(parents=True)
            code.write_text('const char *label = "private_id";\n', encoding="utf-8")
            with (
                patch.object(yoj_scheduler, "ROOT", root),
                patch.object(yoj_scheduler, "public_ready_problem_numbers", return_value={42}),
            ):
                self.assertEqual(yoj_scheduler.pending_unreleased_code_paths([raw_path]), [raw_path])
                self.assertFalse(yoj_scheduler.path_is_publishable(raw_path, {42}, {1717}))
                self.assertTrue(yoj_scheduler.path_is_publishable("代码库/1717_原目录/1717_元数据.json", {42}, {1717}))
                self.assertEqual(yoj_scheduler.scan_for_personal_markers([raw_path]), raw_path)

    def test_target_selector_matches_pending_public_filter_and_excludes_published(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            problems = root / "problems.json"
            snapshot = root / "public.json"
            write_json(problems, {"records": [
                {"problemNo": "42", "public": {"status": "PUBLIC_READY", "onlineVerification": "NO_LOCAL_AC"}},
                {"problemNo": "1717", "public": {"status": "TOPIC_CAPTURED", "onlineVerification": "NO_LOCAL_AC"}},
                {"problemNo": "1718", "public": {"status": "RAW_CAPTURED", "onlineVerification": "ONLINE_ACCEPTED"}},
                {"problemNo": "1719", "public": {"status": "TOPIC_CAPTURED", "onlineVerification": "NO_LOCAL_AC"}},
                {"problemNo": "1720", "public": {"status": "RAW_CAPTURED", "onlineVerification": "ONLINE_SKIPPED"}},
            ]})
            write_json(snapshot, {"records": [{"problemNo": value} for value in (42, 1717, 1718, 1720)]})
            with (
                patch.object(yoj_scheduler, "PROBLEMS_PATH", problems),
                patch.object(yoj_scheduler, "PUBLIC_SNAPSHOT_PATH", snapshot),
                patch.object(yoj_scheduler, "validate_snapshot", side_effect=lambda payload: payload["records"]),
                patch.object(yoj_scheduler, "public_ready_problem_numbers", return_value={42}),
            ):
                self.assertEqual(yoj_scheduler.select_pending_personal_ac_targets(), [1717, 1720])

    def test_no_new_public_id_still_runs_ac_capture_and_verification(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory)
            calls: list[list[str]] = []
            capture_results = iter([
                {"scope": "public_problem_numbers_only", "newProblemNumbers": []},
                {"scope": "public_problem_numbers_and_personal_accepted", "newProblemNumbers": [], "newTopicNumbers": [], "acTargetNumbers": [1717], "refreshedAcNumbers": [1717], "submissionScan": "FULL_TARGETED"},
            ])

            def run_command(command: list[str], *_args: object, **_kwargs: object) -> int:
                calls.append(command)
                return 0

            args = SimpleNamespace(visibility_watch=False, drift_audit=False, sentinel=False, dry_run=False,
                                   request_interval=0, allow_submit=True, max_submissions=50, publish=False, push=False)
            with (
                patch.object(yoj_scheduler, "STATE_DIR", state),
                patch.object(yoj_scheduler, "LOCK_PATH", state / "lock"),
                patch.object(yoj_scheduler, "in_maintenance", return_value=False),
                patch.object(yoj_scheduler, "validate_worktree_before_run", return_value=True),
                patch.object(yoj_scheduler, "verify_pending_remote", return_value=True),
                patch.object(yoj_scheduler, "child_environment", return_value={}),
                patch.object(yoj_scheduler, "prepare_online_environment", return_value={}),
                patch.object(yoj_scheduler, "snapshot_content_digest", return_value="same"),
                patch.object(yoj_scheduler, "run_visibility_audit", return_value=True),
                patch.object(yoj_scheduler, "load_visibility_report", return_value={"hasVisibilityTransitions": False}),
                patch.object(yoj_scheduler, "load_capture_result", side_effect=lambda: next(capture_results)),
                patch.object(yoj_scheduler, "select_pending_personal_ac_targets", return_value=[1717]),
                patch.object(yoj_scheduler, "select_raw_cleanup_backlog", return_value=[]),
                patch.object(yoj_scheduler, "run_command", side_effect=run_command),
                patch.object(yoj_scheduler, "remember_generated_changes"),
                patch.object(yoj_scheduler, "log"),
                patch.dict(yoj_scheduler.os.environ, {"YOJ_SYNC_ENABLE_SUBMIT": "1"}),
            ):
                self.assertEqual(yoj_scheduler.run_once(args), 0)
            capture = next(command for command in calls if "yoj_capture.py" in " ".join(command) and "--discover-only" not in command)
            self.assertEqual(capture[-2:], ["--ac-target", "1717"])
            verification = next(command for command in calls if "online_verify.py" in " ".join(command))
            self.assertIn("--problem", verification)
            self.assertIn("1717", verification)
            self.assertNotIn("--reverify-accepted", verification)


class OnlineResumeTests(unittest.TestCase):
    def test_lost_post_response_persists_uncertain_intent_before_request(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            candidate = root / "candidate.cpp"
            candidate.write_text("int main() {}", encoding="utf-8")
            records: dict = {}
            skips: dict = {}
            saved: list[str] = []
            def request(path: str, data: dict | None = None, *_a: object) -> str:
                if data is not None:
                    self.assertEqual(saved[-1], "SUBMISSION_STATE_UNCERTAIN")
                    raise ConnectionError("lost response")
                return "form"
            with (
                patch.object(sys, "argv", ["online_verify.py", "--problem", "1717", "--submit-interval", "0"]),
                patch.object(online_verify, "ROOT", root),
                patch.object(online_verify, "ensure_report"),
                patch.object(online_verify, "load_context", return_value=({1717: {"problemNo": 1717}}, records, skips)),
                patch.object(online_verify, "read_candidates", return_value={1717: (candidate, "cpp")}),
                patch.object(online_verify, "candidate_paths", return_value=(candidate, candidate)),
                patch.object(online_verify, "local_gate_reasons", return_value=[]),
                patch.object(online_verify, "parse_form", return_value=("/submit", "1717")),
                patch.object(online_verify, "fetch_rows", return_value=[]),
                patch.object(online_verify, "save_report", side_effect=lambda _r, s: saved.append(s[1717]["reason"])),
                patch.object(online_verify, "YoJClient") as client,
            ):
                client.return_value.request.side_effect = request
                self.assertEqual(online_verify.main(), 3)
            self.assertEqual(skips[1717]["reason"], "SUBMISSION_STATE_UNCERTAIN")
            self.assertEqual(skips[1717]["baselineSubmissionNo"], 0)

    def test_uncertain_post_is_not_repeated_even_with_retry_abnormal(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / "candidate.cpp"
            candidate.write_text("int main() {}", encoding="utf-8")
            digest = hashlib.sha256(candidate.read_bytes()).hexdigest()
            skips = {1717: {"candidateSha256": digest, "reason": "SUBMISSION_STATE_UNCERTAIN"}}
            with (
                patch.object(sys, "argv", ["online_verify.py", "--retry-abnormal", "--problem", "1717"]),
                patch.object(online_verify, "ensure_report"),
                patch.object(online_verify, "load_context", return_value=({}, {}, skips)),
                patch.object(online_verify, "read_candidates", return_value={1717: (candidate, "cpp")}),
                patch.object(online_verify, "YoJClient") as client,
            ):
                self.assertEqual(online_verify.main(), 3)
                client.return_value.request.assert_not_called()
                client.return_value.request_json.assert_not_called()

    def test_same_accepted_candidate_is_not_resubmitted(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / "candidate.cpp"
            candidate.write_text("int main() {}", encoding="utf-8")
            digest = hashlib.sha256(candidate.read_bytes()).hexdigest()
            records = {1717: {"candidateSha256": digest, "status": "Accepted"}}
            with (
                patch.object(sys, "argv", ["online_verify.py", "--retry-abnormal", "--problem", "1717"]),
                patch.object(online_verify, "ensure_report"),
                patch.object(online_verify, "load_context", return_value=({}, records, {})),
                patch.object(online_verify, "read_candidates", return_value={1717: (candidate, "cpp")}),
                patch.object(online_verify, "YoJClient") as client,
            ):
                self.assertEqual(online_verify.main(), 0)
                client.return_value.request.assert_not_called()
                client.return_value.request_json.assert_not_called()


if __name__ == "__main__":
    unittest.main()
