"""Authenticated discovery follows advertised entry points, not guessed IDs."""
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools import yoj_capture as capture
from tools import online_verify as online
from tools import yoj_context as context
from tools import build_initial as builder


PREFIX = '/index.php/index/'
LOGOUT = '<a href="/index.php/index/login/logout.html">logout</a>'


def link(path):
    return f'<a href="{path}">entry</a>'


class ContextDiscoveryTests(unittest.TestCase):
    def pages(self):
        index = LOGOUT + link(PREFIX + 'problem/detail/pno/1725.html')
        return {
            PREFIX + 'problem/index.html': index,
            PREFIX + 'problem/index/p/2.html': index,
            PREFIX + 'course/index.html': LOGOUT + '<h2>课程</h2>' + link(PREFIX + 'course/detail/cno/example.html'),
            PREFIX + 'course/detail/cno/example.html': LOGOUT + '<h2>课程主页</h2>' + link(PREFIX + 'contest/index/cno/example.html'),
            PREFIX + 'contest/index.html': LOGOUT + '<table></table>',
            PREFIX + 'contest/index/cno/example.html': LOGOUT + '<table></table>' + link(PREFIX + 'contest/detail/eno/2095.html'),
            PREFIX + 'contest/detail/eno/2095.html': LOGOUT + '<table></table>' + link(PREFIX + 'problem/detail/pno/1726/eno/2095.html'),
        }

    def discover(self, pages, root):
        class Client:
            def login(self):
                pass

            def request(self, path):
                if path not in pages:
                    raise AssertionError('unadvertised request: ' + path)
                page = pages[path]
                return page if page.startswith(('<html', '<!DOCTYPE')) else '<html><body>' + page + '</body></html>'

        with patch.object(capture, 'YoJClient', Client), \
             patch.object(capture, 'existing_context', return_value=({}, {}, {1725})), \
             patch.object(capture, 'STATE_DIR', root), \
             patch.object(capture, 'CAPTURE_RESULT_PATH', root / 'result.json'), \
             patch.object(sys, 'argv', ['capture', '--account-visible', '--discover-only', '--request-interval', '0']):
            return capture.main()

    def test_course_exam_context_discovers_problem_missing_from_main_index(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.assertEqual(self.discover(self.pages(), root), 0)
            result = json.loads((root / 'result.json').read_text())
            self.assertEqual(result['newProblemNumbers'], [1726])
            snapshot = json.loads((root / 'account-visible-problems.json').read_text())
            self.assertEqual(snapshot['problemPaths']['1726'], [PREFIX + 'problem/detail/pno/1726/eno/2095.html'])
            self.assertEqual(snapshot['problemNumbers'], [1725, 1726])

    def test_pagination_keeps_course_filter_and_never_follows_external_or_join_link(self):
        pages = self.pages()
        pages[PREFIX + 'contest/index/cno/example.html'] = (
            LOGOUT + '<table></table>'
            + link(PREFIX + 'contest/index/cno/example/p/2.html')
            + link('https://invalid.example/index.php/index/contest/detail/eno/999.html')
            + link(PREFIX + 'contest/enroll/eno/999.html')
        )
        pages[PREFIX + 'contest/index/cno/example/p/2.html'] = (
            LOGOUT + '<table></table>' + link(PREFIX + 'contest/detail/eno/2095.html')
        )
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.assertEqual(self.discover(pages, root), 0)
            self.assertEqual(json.loads((root / 'result.json').read_text())['newProblemNumbers'], [1726])

    def test_malformed_course_response_does_not_publish_partial_snapshot(self):
        pages = self.pages()
        pages[PREFIX + 'course/detail/cno/example.html'] = '<html>temporary failure</html>'
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            snapshot = root / 'account-visible-problems.json'
            snapshot.write_text('previous valid evidence')
            with self.assertRaises(RuntimeError):
                self.discover(pages, root)
            self.assertEqual(snapshot.read_text(), 'previous valid evidence')

    def test_empty_real_course_container_is_a_complete_empty_list(self):
        pages = self.pages()
        pages[PREFIX + 'course/index.html'] = LOGOUT + '<div class="ui main container"></div>'
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.assertEqual(self.discover(pages, root), 0)

    def test_course_homework_detail_can_link_to_an_exam(self):
        pages = self.pages()
        pages[PREFIX + 'course/detail/cno/example.html'] = LOGOUT + '<h4>课程主页</h4>' + link(PREFIX + 'homework/index/cno/example.html')
        pages[PREFIX + 'homework/index/cno/example.html'] = LOGOUT + '<table></table>' + link(PREFIX + 'homework/detail/cno/example/hno/25.html')
        pages[PREFIX + 'homework/detail/cno/example/hno/25.html'] = LOGOUT + '<h4>作业</h4>' + link(PREFIX + 'contest/detail/eno/2095.html')
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.assertEqual(self.discover(pages, root), 0)
            self.assertEqual(json.loads((root / 'result.json').read_text())['newProblemNumbers'], [1726])

    def test_snapshot_rejects_mismatched_problem_or_external_context(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.discover(self.pages(), root)
            path = root / 'account-visible-problems.json'
            snapshot = json.loads(path.read_text())
            for bad in [PREFIX + 'problem/detail/pno/1727/eno/2095.html', 'https://invalid.example/index.php/index/problem/detail/pno/1726.html']:
                snapshot['problemPaths']['1726'] = [bad]
                path.write_text(json.dumps(snapshot))
                with self.assertRaises((RuntimeError, ValueError)):
                    capture.load_account_snapshot(path)

    def test_snapshot_cannot_silently_drop_contexts(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.discover(self.pages(), root)
            path = root / 'account-visible-problems.json'
            snapshot = json.loads(path.read_text())
            snapshot['problemPaths']['1726'] = []
            path.write_text(json.dumps(snapshot))
            with self.assertRaises(RuntimeError):
                capture.load_account_snapshot(path)

    def test_relative_pagination_is_resolved_against_its_entry_page(self):
        pages = self.pages()
        pages[PREFIX + 'contest/index.html'] = LOGOUT + '<table></table>' + link('index/p/2.html')
        pages[PREFIX + 'contest/index/p/2.html'] = LOGOUT + '<table></table>'
        with tempfile.TemporaryDirectory() as temporary:
            self.assertEqual(self.discover(pages, Path(temporary)), 0)

    def test_query_pagination_preserves_course_scope(self):
        pages = self.pages()
        pages[PREFIX + 'contest/index/cno/example.html'] = LOGOUT + '<table></table>' + link('?p=2')
        pages[PREFIX + 'contest/index/cno/example.html?p=2'] = LOGOUT + '<table></table>' + link(PREFIX + 'contest/detail/eno/2095.html')
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.assertEqual(self.discover(pages, root), 0)
            self.assertEqual(json.loads((root / 'result.json').read_text())['newProblemNumbers'], [1726])

    def test_main_index_limit_cannot_claim_complete_account_discovery(self):
        page = '<html><body>' + link(PREFIX + 'problem/detail/pno/1725.html') + link(PREFIX + 'problem/index/p/2.html') + '</body></html>'
        class Client:
            def request(self, path):
                return page
        with self.assertRaisesRegex(RuntimeError, 'PAGE_LIMIT'):
            capture.all_public_problem_numbers(Client(), capture.RateLimiter(0), 1, contexts={})

    def test_discovery_budget_stops_without_overwriting_previous_snapshot(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            snapshot = root / 'account-visible-problems.json'
            snapshot.write_text('previous valid evidence')
            pages = self.pages()
            class Client:
                def request(self, path):
                    return '<html><body>' + pages[path] + '</body></html>'
            with self.assertRaisesRegex(RuntimeError, 'PAGE_LIMIT'):
                context.discover_account_contexts(Client(), capture.RateLimiter(0), {1725}, 2)
            self.assertEqual(snapshot.read_text(), 'previous valid evidence')

    def test_private_coverage_cannot_be_missing_from_version_two_snapshot(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.discover(self.pages(), root)
            path = root / 'account-visible-problems.json'
            snapshot = json.loads(path.read_text())
            snapshot['coverage'] = {}
            path.write_text(json.dumps(snapshot))
            with self.assertRaises(RuntimeError):
                capture.load_account_snapshot(path)

    def test_ac_capture_materializes_the_discovered_context_instead_of_bare_url(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            snapshot_path = root / 'account-visible-problems.json'
            url = PREFIX + 'problem/detail/pno/1726/eno/2095.html'
            snapshot_path.write_text(json.dumps({'schemaVersion': 2, 'scope': 'account_visible',
                'capturedAt': capture.utc_now(), 'pagesFetched': 3, 'problemNumbers': [1726],
                'problemPaths': {'1726': [url]}, 'coverage': {'entryPages': 2, 'courses': 0, 'exams': 1, 'homework': 0, 'inaccessibleEntries': 0}}))
            manifest_path = root / 'manifest.json'
            manifest_path.write_text('{"problems":[],"totals":{}}')
            class Client:
                def login(self):
                    pass
                def request(self, path, data=None, referer=''):
                    if 'getcode' in path:
                        return '{"status":1,"code":"int main(){}"}'
                    if path == url:
                        return '<html><body><h1>#1726. Example</h1><div id="p_content">statement</div></body></html>'
                    if 'submissions/detail/' in path:
                        return '<html>submission detail</html>'
                    raise AssertionError('context was stripped: ' + path)
            rows = [{'problemNo': 1726, 'submissionNo': 1001, 'status': 'Accepted', 'language': 'cpp17'}]
            with patch.object(capture, 'ROOT', root), patch.object(capture, 'STATE_DIR', root), \
                 patch.object(capture, 'CAPTURE_RESULT_PATH', root / 'result.json'), \
                 patch.object(capture, 'MANIFEST_PATH', manifest_path), \
                 patch.object(capture, 'existing_context', return_value=({'problems': [], 'totals': {}}, {}, set())), \
                 patch.object(capture, 'all_submission_rows', return_value=rows), patch.object(capture, 'YoJClient', Client), \
                 patch.object(sys, 'argv', ['capture', '--account-visible', '--account-snapshot', str(snapshot_path), '--request-interval', '0']):
                self.assertEqual(capture.main(), 0)
            manifest = json.loads(manifest_path.read_text())
            row = manifest['problems'][0]
            self.assertEqual(row['problemUrl'], context.BASE + url)
            metadata_path = root / '代码库' / row['files']['metadata']
            self.assertEqual(json.loads(metadata_path.read_text())['problem']['problemUrl'], context.BASE + url)

    def test_context_validation_rejects_tokens_foreign_hosts_and_wrong_problem(self):
        for value in ['https://invalid.example/index.php/index/problem/detail/pno/1726.html',
                      PREFIX + 'problem/detail/pno/1726/eno/2095.html?token=secret',
                      PREFIX + 'problem/detail/pno/1727/eno/2095.html',
                      PREFIX + 'contest/enroll/eno/2095.html']:
            with self.subTest(value=value), self.assertRaises(ValueError):
                context.problem_path(1726, value)

    def test_truncated_authenticated_page_does_not_overwrite_evidence(self):
        pages = self.pages()
        pages[PREFIX + 'course/index.html'] = '<html><body>' + LOGOUT + '<h2>课程</h2><table>'
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            snapshot = root / 'account-visible-problems.json'
            snapshot.write_text('previous valid evidence')
            with self.assertRaises(RuntimeError):
                self.discover(pages, root)
            self.assertEqual(snapshot.read_text(), 'previous valid evidence')

    def test_child_exam_not_started_label_does_not_hide_accessible_siblings(self):
        pages = self.pages()
        pages[PREFIX + 'course/detail/cno/example.html'] += '<span>另一个考试尚未开始</span>'
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.assertEqual(self.discover(pages, root), 0)
            self.assertEqual(json.loads((root / 'result.json').read_text())['newProblemNumbers'], [1726])

    def test_main_index_context_is_not_replaced_by_a_guessed_bare_path(self):
        pages = self.pages()
        index = pages[PREFIX + 'problem/index.html'] + link(PREFIX + 'problem/detail/pno/1726/eno/2095.html')
        pages[PREFIX + 'problem/index.html'] = pages[PREFIX + 'problem/index/p/2.html'] = index
        pages[PREFIX + 'course/index.html'] = LOGOUT + '<table></table>'
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.assertEqual(self.discover(pages, root), 0)
            snapshot = json.loads((root / 'account-visible-problems.json').read_text())
            self.assertEqual(snapshot['problemPaths'].get('1726'), [PREFIX + 'problem/detail/pno/1726/eno/2095.html'])

    def run_unchanged_ac_capture(self, *, advertised_bare=False, truncated=False):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            url = PREFIX + 'problem/detail/pno/1726/eno/2095.html'
            bare = context.BASE + PREFIX + 'problem/detail/pno/1726.html'
            folder = root / '代码库' / '1726_example'
            folder.mkdir(parents=True)
            code = folder / 'code.cpp'
            code.write_text('original code must stay unchanged')
            metadata = folder / 'metadata.json'
            metadata.write_text(json.dumps({'problem': {'problemUrl': bare}}))
            statement = folder / 'problem.html'
            statement.write_text('previous valid statement')
            entry = {'problemNo': '1726', 'submissionNo': '1001', 'problemUrl': bare,
                     'folder': '1726_example', 'title': 'example', 'files': {
                         'metadata': '1726_example/metadata.json', 'problem': '1726_example/problem.html',
                         'completeCode': '1726_example/code.cpp'}}
            manifest = {'problems': [entry], 'totals': {}}
            snapshot = root / 'snapshot.json'
            snapshot.write_text(json.dumps({'schemaVersion': 2, 'scope': 'account_visible', 'capturedAt': capture.utc_now(),
                'pagesFetched': 3, 'problemNumbers': [1726], 'problemPaths': {'1726': ([PREFIX + 'problem/detail/pno/1726.html'] if advertised_bare else []) + [url]},
                'coverage': {'entryPages': 2, 'courses': 0, 'exams': 1, 'homework': 0, 'inaccessibleEntries': 0}}))
            class Client:
                def login(self):
                    pass
                def request(self, path, *args):
                    if path == PREFIX + 'problem/detail/pno/1726.html':
                        return '<html><body><div class="system-message">看不到</div></body></html>'
                    if path == url:
                        if truncated:
                            return '<html><body><h1>#1726. Example</h1><div id="p_content">truncated'
                        return '<html><body><h1>#1726. Example</h1><div id="p_content">statement</div></body></html>'
                    raise AssertionError('unchanged AC source must not be fetched again')
            with patch.object(capture, 'ROOT', root), patch.object(capture, 'STATE_DIR', root), \
                 patch.object(capture, 'CAPTURE_RESULT_PATH', root / 'result.json'), \
                 patch.object(capture, 'MANIFEST_PATH', root / 'manifest.json'), \
                 patch.object(capture, 'existing_context', return_value=(manifest, {1726: entry}, {1726})), \
                 patch.object(capture, 'frozen_public_ready_numbers', return_value=set()), \
                 patch.object(capture, 'all_submission_rows', return_value=[{'problemNo': 1726, 'submissionNo': 1001, 'status': 'Accepted'}]), \
                 patch.object(capture, 'YoJClient', Client), \
                 patch.object(sys, 'argv', ['capture', '--account-visible', '--account-snapshot', str(snapshot), '--ac-target', '1726', '--request-interval', '0']):
                if truncated:
                    with self.assertRaises(RuntimeError):
                        capture.main()
                    self.assertEqual(statement.read_text(), 'previous valid statement')
                    self.assertEqual(json.loads(metadata.read_text())['problem']['problemUrl'], bare)
                    return
                self.assertEqual(capture.main(), 0)
            self.assertEqual(json.loads(metadata.read_text())['problem']['problemUrl'], context.BASE + url)
            self.assertEqual(code.read_text(), 'original code must stay unchanged')
            self.assertEqual(json.loads((root / 'result.json').read_text())['contextUpdatedNumbers'], [1726])

    def test_same_ac_still_refreshes_unpublished_topic_context_without_replacing_code(self):
        self.run_unchanged_ac_capture()

    def test_advertised_bare_route_is_checked_before_skipping_context_refresh(self):
        self.run_unchanged_ac_capture(advertised_bare=True)

    def test_truncated_context_refresh_preserves_statement_and_metadata(self):
        self.run_unchanged_ac_capture(truncated=True)


class ContextVerificationTests(unittest.TestCase):
    def test_online_verifier_preserves_eno_for_form_and_post_referer(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate = root / 'candidate.cpp'
            candidate.write_text('int main() {}\n')
            path = PREFIX + 'problem/detail/pno/1726/eno/2095.html'
            records, skips = {}, {}
            submitted = []
            form = ('<form id="submit_code" action="/index.php/index/index/prob_submit.html">'
                    '<input name="pid" value="1726"><input name="code">'
                    '<div id="editor"></div><div data-value="cpp17"></div></form>')

            class Client:
                def login(self):
                    pass

                def request(self, url, data=None, referer=''):
                    if data and 'getcode' in url:
                        return json.dumps({'status': 1, 'code': candidate.read_text()})
                    if data:
                        if referer != path:
                            raise AssertionError('context stripped from POST referer')
                        submitted.append(data)
                        return '{"status":1}'
                    return form if url == path else '<h1>看不到</h1>'

            row = {'problemNo': 1726, 'submissionNo': 3001, 'status': 'Accepted', 'score': 100,
                   'timeMs': 1, 'memoryKiB': 1024, 'submittedBytes': len(candidate.read_bytes())}
            with patch.object(sys, 'argv', ['online', '--problem', '1726', '--submit-interval', '0']), \
                 patch.object(online, 'ROOT', root), patch.object(online, 'ensure_report'), \
                 patch.object(online, 'load_context', return_value=({1726: {'problemNo': 1726, 'problemUrl': context.BASE + path}}, records, skips)), \
                 patch.object(online, 'read_candidates', return_value={1726: (candidate, 'cpp17')}), \
                 patch.object(online, 'candidate_paths', return_value=(candidate, candidate)), \
                 patch.object(online, 'local_gate_reasons', return_value=[]), \
                 patch.object(online, 'fetch_rows', side_effect=lambda _c: [row] if submitted else []), \
                 patch.object(online, 'save_report'), patch.object(online, 'rebuild_public_index'), \
                 patch.object(online, 'YoJClient', Client):
                self.assertEqual(online.main(), 0)
            self.assertIn(1726, records)
            self.assertTrue(records[1726]['roundTrip']['exactByteMatch'])
            self.assertEqual(len(submitted), 1)

    def test_quick_submit_link_preserves_verified_context(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            code = root / 'code.cpp'
            code.write_text('int main(){}')
            record = {'problemNo': 1726, 'problemUrl': context.BASE + PREFIX + 'problem/detail/pno/1726/eno/2095.html',
                      'public': {'status': 'PUBLIC_READY', 'directlySubmittableCode': 'code.cpp'}}
            ready = {'directlySubmittableCode': 'code.cpp', 'directlySubmittableCodeSha256': builder.source_file_hash(code), 'language': 'cpp17'}
            with patch.object(builder, 'ROOT', root), patch.object(builder, 'load_public_ready', return_value={'1726': ready}), \
                 patch.object(builder, 'load_online_public_numbers', return_value=set()):
                manifest = builder.make_quick_submit_manifest([record], {})
            self.assertEqual(manifest['entries'][0]['problemUrl'], record['problemUrl'])


if __name__ == '__main__':
    unittest.main()
