"""Real-template fragments must earn the same local and publication gates."""

import hashlib
import importlib
import importlib.util
import json
import re
import sys
import tempfile
import unittest
from copy import deepcopy
from types import SimpleNamespace
from contextlib import ExitStack
from pathlib import Path
from unittest.mock import patch

from tools import build_initial, compile_candidates, online_verify, publish_ready, release_gate, run_samples, sanitize_candidates, validate_candidates


class FillInTests(unittest.TestCase):
    number = 285
    template = '#include <stdio.h>\nint main() {\n____qcodep____\n}\n'
    fragment = 'printf("2\\n");\nreturn 0;\n'
    program = '#include <stdio.h>\nint main() {\nprintf("2\\n");\nreturn 0;\n\n}\n'

    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.raw = self.root / '代码库'
        self.cleaned = self.root / 'staging/cleaned-code'
        self.folder = f'{self.number:04d}_fixture'
        self.complete = self.folder + f'/{self.number:04d}_提交_1_cpp_完整代码.cpp'
        self.direct = self.folder + f'/{self.number:04d}_提交_1_cpp_可提交代码.cpp'
        self.statement = self.root / '题解' / self.folder / f'{self.number:04d}_题目.md'
        self.asset = self.statement.parent / 'assets/template.cpp'
        for path, content in [(self.raw / self.complete, self.fragment),
                              (self.raw / self.direct, self.fragment),
                              (self.cleaned / self.complete, self.program),
                              (self.cleaned / self.direct, self.fragment),
                              (self.asset, self.template),
                              (self.statement, '# Fixture\n\n## 输入样例\n\n```text\n1\n```\n\n## 输出样例\n\n```text\n2\n```\n')]:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content, encoding='utf-8')
        self.record = {
            'problemNo': str(self.number), 'title': 'Fixture', 'folder': self.folder, 'language': 'cpp',
            'archive': {'completeCode': '代码库/' + self.complete,
                        'directlySubmittableCode': '代码库/' + self.direct},
            'public': {'statement': self.statement.relative_to(self.root).as_posix()},
            'statement': {'assets': [{'url': f'http://yoj.ruc.edu.cn/index.php/index/problem/downloadtk/pno/{self.number}.html',
                'relativePath': 'assets/template.cpp', 'status': 'CACHED',
                'sha256': hashlib.sha256(self.template.encode()).hexdigest(),
                'retrievedFrom': {'kind': 'YOJ_PUBLIC_TEMPLATE_API', 'problemNo': self.number,
                    'url': 'http://yoj.ruc.edu.cn/index.php/index/problem/gettkcode.html', 'method': 'POST'}}]},
        }
        self.stack = ExitStack()
        self.addCleanup(self.stack.close)
        for module in (validate_candidates, compile_candidates, run_samples):
            for name, value in {'ROOT': self.root, 'RAW_ROOT': self.raw,
                                'CLEANED_ROOT': self.cleaned,
                                'REPAIRED_ROOT': self.root / 'staging/repaired-code',
                                'PORTABLE_ROOT': self.root / 'staging/cpp17-portable'}.items():
                self.stack.enter_context(patch.object(module, name, value))
            normal = 'CANDIDATE_ROOT' if module is validate_candidates else 'NORMALIZED_ROOT'
            self.stack.enter_context(patch.object(module, normal, self.root / 'staging/cpp17-compatible'))
        work = self.root / 'staging/sample-tests'
        work.mkdir()
        self.stack.enter_context(patch.object(run_samples, 'WORK_ROOT', work))

    def helper(self):
        spec = importlib.util.find_spec('tools.assemble_fill_in_candidates')
        self.assertIsNotNone(spec, 'verified template assembly is not implemented')
        return importlib.import_module('tools.assemble_fill_in_candidates')

    def local_reports(self, category='PASS'):
        reports = {}
        for kind in ('validation', 'compile', 'samples'):
            path = self.root / f'{kind}.json'
            path.write_text(json.dumps({'results': [{'problemNo': str(self.number), 'category': category,
                'candidateSha256': hashlib.sha256((self.cleaned / self.complete).read_bytes()).hexdigest()}]}))
            reports[kind] = path
        sanitization = self.root / 'sanitize.json'
        sanitization.write_text(json.dumps({'fileRecords': [{'candidate': 'staging/cleaned-code/' + name,
            'candidateSha256': hashlib.sha256((self.cleaned / name).read_bytes()).hexdigest(),
            'remainingConfiguredMarkers': []} for name in (self.complete, self.direct)]}))
        reports['sanitization'] = sanitization
        return reports

    def run_online(self, *, category='PASS', previous=None, uncertain=False, live_template=None,
                   ordinary_form=False, lose_response=False):
        """Real verifier/gates/record parser with only YOJ transport replaced."""
        records = {self.number: previous} if previous else {}
        skips = {self.number: {'reason': 'SUBMISSION_STATE_UNCERTAIN',
                 'candidateSha256': hashlib.sha256(self.fragment.encode()).hexdigest()}} if uncertain else {}
        submitted = []
        requests = []
        fragment, number = self.fragment, self.number
        template = self.template if live_template is None else live_template
        context = f'/index.php/index/problem/detail/pno/{number}/eno/99.html'
        self.record['problemUrl'] = 'http://yoj.ruc.edu.cn' + context

        class Client:
            def login(self):
                pass

            def request(self, url, data=None, referer='', **kwargs):
                requests.append((url, data, referer))
                if 'getcode.html' in url:
                    return json.dumps({'status': 1, 'code': fragment})
                if url == '/index.php/index/submissions/index.html':
                    if not submitted:
                        return '<table></table>'
                    return (f'<tr><a href="/submissions/detail/subno/100.html">100</a>'
                            f'<a href="/problem/detail/pno/{number}.html">Fixture</a>'
                            '<span class="status">Accepted</span><span class="score">100</span>'
                            '<a>cpp</a> / 24 B 1 ms 1024 KB</tr>')
                if url == context:
                    if ordinary_form:
                        return (f'<form id="submit_code" action="/index.php/index/index/prob_submit.html">'
                                f'<input name="pid" value="{number}"><input name="code">'
                                '<div id="editor"></div><div data-value="cpp"></div></form><h2>填空题</h2>')
                    return '<h1>部分代码提交</h1>'
                raise AssertionError('unexpected request')

            def request_json(self, url, data=None, referer='', **kwargs):
                requests.append((url, data, referer))
                if url == '/index.php/index/problem/gettkcode.html':
                    return {'status': 1, 'content': template}
                if url == '/index.php/index/index/prob_submit_tk.html':
                    submitted.append((data, referer))
                    if lose_response:
                        raise TimeoutError('lost response')
                    return {'status': 1}
                raise AssertionError('unexpected JSON request')

        report = self.root / 'staging/online.json'
        with ExitStack() as patches:
            for module, name, value in [
                (release_gate, 'ROOT', self.root), (release_gate, 'RAW_ROOT', self.raw),
                (release_gate, '_CANDIDATE_ROOTS', (self.cleaned,)),
                (release_gate, 'REPORT_PATHS', self.local_reports(category)),
                (online_verify, 'ROOT', self.root), (online_verify, 'REPORT_PATH', report),
                (online_verify, 'YoJClient', Client),
            ]:
                patches.enter_context(patch.object(module, name, value))
            patches.enter_context(patch.object(online_verify, 'load_context',
                return_value=({number: self.record}, records, skips)))
            patches.enter_context(patch.object(online_verify, 'read_candidates',
                return_value={number: (self.cleaned / self.direct, 'cpp')}))
            patches.enter_context(patch.object(sys, 'argv', ['online', '--retry-abnormal', '--submit-interval', '0']))
            result = online_verify.main()
        return result, records, skips, submitted, requests

    def test_real_template_assembled_candidate_passes_syntax(self):
        self.assertEqual(validate_candidates.validate_record(self.record)['category'], 'PASS')

    def test_real_template_assembled_candidate_compiles_and_links(self):
        data = self.root / 'problems.json'
        report = self.root / 'compile.json'
        data.write_text(json.dumps({'records': [self.record]}))
        with patch.object(compile_candidates, 'DATA_PATH', data), \
             patch.object(compile_candidates, 'REPORT_PATH', report), \
             patch.object(sys, 'argv', ['compile']):
            self.assertEqual(compile_candidates.main(), 0)
        result = json.loads(report.read_text())['results'][0]
        self.assertEqual(result['category'], 'PASS')
        self.assertEqual(result['candidateSha256'], hashlib.sha256(self.program.encode()).hexdigest())

    def test_real_template_assembled_program_runs_sample(self):
        self.assertEqual(run_samples.test_record(self.record, run=True)['category'], 'PASS')

    def test_fragment_without_assembled_program_still_blocks(self):
        (self.cleaned / self.complete).write_text(self.fragment)
        self.assertEqual(validate_candidates.validate_record(self.record)['category'], 'FILL_IN_FRAGMENT')

    def test_modified_or_untrusted_template_still_blocks(self):
        self.asset.write_text(self.template + '// changed')
        self.assertEqual(validate_candidates.validate_record(self.record)['category'], 'FILL_IN_FRAGMENT')
        self.asset.write_text(self.template)
        self.record['statement']['assets'][0]['url'] = 'https://other.example/template.cpp'
        self.assertEqual(validate_candidates.validate_record(self.record)['category'], 'FILL_IN_FRAGMENT')

    def test_assembly_preserves_original_fragment_and_is_idempotent(self):
        helper = self.helper()
        original = (self.raw / self.complete).read_bytes()
        helper.prepare_record(self.record, self.root)
        output = self.root / 'staging/repaired-code' / self.complete
        self.assertEqual(output.read_text(), self.program)
        self.assertEqual((self.raw / self.complete).read_bytes(), original)
        helper.prepare_record(self.record, self.root)
        self.assertEqual(output.read_text(), self.program)

    def test_assembly_rejects_multiple_slots_and_path_escape(self):
        helper = self.helper()
        asset = self.record['statement']['assets'][0]
        self.asset.write_text(self.template + '____qcodep____')
        asset['sha256'] = hashlib.sha256(self.asset.read_bytes()).hexdigest()
        with self.assertRaises(ValueError):
            helper.prepare_record(self.record, self.root)
        asset['relativePath'] = '../../../../outside.cpp'
        with self.assertRaises(ValueError):
            helper.prepare_record(self.record, self.root)

    def test_live_payload_keeps_fragment_bytes_and_matches_compiled_program(self):
        verify = getattr(online_verify, 'verify_assembled_payload', None)
        self.assertTrue(callable(verify), 'live framework must match the locally compiled program')
        blocks, program = verify(self.template, [self.fragment.rstrip('\n')],
                                 self.cleaned / self.complete, self.cleaned / self.direct)
        self.assertEqual(blocks, [self.fragment])
        self.assertEqual(program, self.program.encode())

    def test_changed_live_framework_cannot_be_submitted_as_verified_assembly(self):
        verify = getattr(online_verify, 'verify_assembled_payload', None)
        self.assertTrue(callable(verify), 'live framework must match the locally compiled program')
        with self.assertRaisesRegex(ValueError, 'ASSEMBL'):
            verify(self.template.replace('int main()', 'int main(int argc, char **argv)'),
                   [self.fragment], self.cleaned / self.complete, self.cleaned / self.direct)

    def test_online_index_refresh_keeps_frozen_and_offline_resource_evidence(self):
        commands = []
        def child(command, **kwargs):
            commands.append(command)
            return SimpleNamespace(returncode=0, stdout='', stderr='')
        with patch.object(online_verify.subprocess, 'run', side_effect=child):
            online_verify.rebuild_public_index()
        self.assertEqual(len(commands), 1)
        self.assertIn('--preserve-frozen', commands[0])
        self.assertIn('--no-download-assets', commands[0])

    def test_published_fragment_uses_the_verified_partial_submission_endpoint(self):
        code = self.statement.parent / f'{self.number:04d}_填空提交片段.cpp'
        code.write_text(self.fragment)
        reference = code.relative_to(self.root).as_posix()
        ready = self.root / 'ready.json'
        ready.write_text(json.dumps({'records': [{'problemNo': self.number, 'status': 'PUBLIC_READY',
            'directlySubmittableCode': reference, 'directlySubmittableCodeSha256': hashlib.sha256(code.read_bytes()).hexdigest(),
            'submissionKind': 'PARTIAL_CODE', 'language': 'cpp'}]}))
        self.record['public'].update({'status': 'PUBLIC_READY', 'directlySubmittableCode': reference})
        with patch.object(build_initial, 'ROOT', self.root), \
             patch.object(build_initial, 'PUBLIC_READY_PATH', ready), \
             patch.object(build_initial, 'ONLINE_PUBLIC_PATH', self.root / 'absent.json'):
            item = build_initial.make_quick_submit_manifest([self.record], {})['entries'][0]
        self.assertEqual(item.get('submissionKind'), 'PARTIAL_CODE')
        self.assertTrue(item['submitEndpoint'].endswith('/prob_submit_tk.html'))

    def test_release_materializes_separate_files_without_overwriting_raw_archive(self):
        reports = self.local_reports()
        ready_path = self.root / 'ready.json'
        online = {'problemNo': self.number, 'status': 'Accepted', 'submissionNo': 100,
                  'submissionKind': 'PARTIAL_CODE', 'partialTemplateVerified': True,
                  'candidateSha256': hashlib.sha256(self.fragment.encode()).hexdigest(),
                  'assembledProgramSha256': hashlib.sha256(self.program.encode()).hexdigest(),
                  'roundTrip': {'status': 'VISIBLE', 'sourceVisibleInSubmissionDetail': True, 'exactByteMatch': True}}
        with patch.object(release_gate, 'ROOT', self.root), \
             patch.object(release_gate, 'RAW_ROOT', self.raw), \
             patch.object(release_gate, '_CANDIDATE_ROOTS', (self.cleaned,)), \
             patch.object(release_gate, 'REPORT_PATHS', reports), \
             patch.object(publish_ready, 'ROOT', self.root), \
             patch.object(publish_ready, 'PUBLIC_READY_PATH', ready_path), \
             patch.object(publish_ready, 'BACKUP_ROOT', self.root / '.yoj-sync/backup'):
            legacy = dict(online)
            legacy.pop('assembledProgramSha256')
            unverified, reasons = publish_ready.ready_record(self.record, legacy, None)
            self.assertIsNone(unverified, 'old fragment AC must not certify a new assembled program')
            self.assertIn('ONLINE_TEMPLATE_ASSEMBLY_NOT_VERIFIED', reasons)
            for field in ('submissionKind', 'partialTemplateVerified'):
                incomplete = dict(online)
                incomplete.pop(field)
                unverified, reasons = publish_ready.ready_record(self.record, incomplete, None)
                self.assertIsNone(unverified, 'hash alone must not replace actual live-template verification')
            asset = self.record['statement']['assets'][0]
            original_url = asset['url']
            asset['url'] = 'https://other.example/template.cpp'
            untrusted, reasons = publish_ready.ready_record(self.record, online, None)
            self.assertIsNone(untrusted, 'missing template provenance must not fall back to overwriting raw files')
            asset['url'] = original_url
            ready, reasons = publish_ready.ready_record(self.record, online, None)
            self.assertEqual(reasons, [])
            self.assertTrue(ready['completeCode'].startswith('题解/'))
            self.assertTrue(ready['directlySubmittableCode'].startswith('题解/'))
            publish_ready.apply_release([self.record], {str(self.number): ready}, 'fixture')
        self.assertEqual((self.raw / self.complete).read_text(), self.fragment)
        self.assertEqual((self.raw / self.direct).read_text(), self.fragment)
        self.assertEqual((self.root / ready['completeCode']).read_text(), self.program)
        self.assertEqual((self.root / ready['directlySubmittableCode']).read_text(), self.fragment)

    def test_local_fragment_gate_cannot_be_overridden_to_submit(self):
        (self.cleaned / self.complete).write_text(self.fragment)
        result, records, skips, submitted, requests = self.run_online(category='FILL_IN_FRAGMENT')
        self.assertEqual(result, 0)
        self.assertEqual(submitted, [])
        self.assertEqual(requests, [])
        self.assertFalse(records)
        self.assertTrue(skips[self.number]['reason'].startswith('LOCAL_GATE_'))

    def test_live_partial_submission_records_assembly_hash_and_context(self):
        result, records, skips, submitted, _ = self.run_online()
        self.assertEqual(result, 0)
        self.assertEqual(skips, {})
        self.assertEqual(submitted, [({'pno': str(self.number), 'codes': json.dumps([self.fragment]), 'compiler': 'cpp'},
                                     f'/index.php/index/problem/detailtk/pno/{self.number}/eno/99.html')])
        self.assertEqual(records[self.number]['assembledProgramSha256'], hashlib.sha256(self.program.encode()).hexdigest())
        self.assertTrue(records[self.number]['roundTrip']['exactByteMatch'])

    def test_changed_live_template_sends_no_judge_submission(self):
        result, records, skips, submitted, _ = self.run_online(live_template=self.template + '// changed')
        self.assertEqual(result, 0)
        self.assertFalse(records)
        self.assertFalse(submitted)
        self.assertIn('LIVE_TEMPLATE_MISMATCH', skips[self.number]['reason'])

    def test_assembled_fragment_must_not_use_an_ordinary_submit_form(self):
        result, records, skips, submitted, _ = self.run_online(ordinary_form=True)
        self.assertEqual(result, 0)
        self.assertFalse(records)
        self.assertFalse(submitted)
        self.assertEqual(skips[self.number]['reason'], 'PARTIAL_ASSEMBLY_FORM_CHANGED')

    def test_uncertain_submission_is_never_reposted(self):
        result, _, _, submitted, _ = self.run_online(uncertain=True)
        self.assertEqual(result, 3)
        self.assertFalse(submitted)

    def test_lost_post_response_retains_uncertain_checkpoint(self):
        result, _, skips, submitted, _ = self.run_online(lose_response=True)
        self.assertEqual(result, 3)
        self.assertEqual(len(submitted), 1)
        self.assertEqual(skips[self.number]['reason'], 'SUBMISSION_STATE_UNCERTAIN')


class GeneralFillInTests(FillInTests):
    """The same contract must work for a new ID, not just the legacy pair."""
    number = 2047

    def run_assembler(self, records):
        data = self.root / 'data/problems.json'
        data.parent.mkdir(exist_ok=True)
        data.write_text(json.dumps({'records': records}))
        with patch.object(self.helper(), 'ROOT', self.root), patch.object(sys, 'argv', ['assemble']):
            result = self.helper().main()
        return result, json.loads((self.root / 'staging/template-assembly-report.json').read_text())

    def test_cli_discovers_new_single_slot_without_problem_selector(self):
        result, report = self.run_assembler([self.record])
        self.assertEqual(result, 0)
        self.assertEqual(report['records'][0]['status'], 'ASSEMBLED')
        self.assertEqual(report['records'][0]['problemNo'], self.number)
        self.assertEqual((self.root / 'staging/repaired-code' / self.complete).read_text(), self.program)

    def test_failed_template_is_reported_and_does_not_prevent_other_records(self):
        blocked = deepcopy(self.record)
        blocked['problemNo'] = '2048'
        blocked['problemUrl'] = 'http://yoj.ruc.edu.cn/index.php/index/problem/detailtk/pno/2048/eno/99.html'
        blocked['statement']['assets'] = []
        result, report = self.run_assembler([blocked, self.record])
        self.assertEqual(result, 0)
        self.assertEqual([(r['problemNo'], r['status']) for r in report['records']],
                         [(2048, 'BLOCKED'), (2047, 'ASSEMBLED')])
        self.assertTrue(report['records'][0]['reason'].startswith('FILL_IN_'))

    def test_ordinary_complete_program_is_not_assembled(self):
        self.record['statement']['assets'] = []
        for name in (self.complete, self.direct):
            (self.raw / name).write_text(self.program)
        _, report = self.run_assembler([self.record])
        self.assertEqual(report['records'], [])
        self.assertFalse((self.root / 'staging/repaired-code' / self.complete).exists())

    def test_missing_asset_still_blocks_a_fragment_even_without_type_metadata(self):
        self.record['statement']['assets'] = []
        self.assertEqual(validate_candidates.validate_record(self.record)['category'], 'FILL_IN_FRAGMENT')

    def test_single_slot_c_template_is_supported(self):
        self.record['language'] = 'c'
        self.helper().prepare_record(self.record, self.root)
        self.assertTrue(self.helper().is_assembled_candidate(self.record, self.cleaned / self.complete, self.root))
        with patch.object(release_gate, 'ROOT', self.root), \
             patch.object(release_gate, 'RAW_ROOT', self.raw), \
             patch.object(release_gate, '_CANDIDATE_ROOTS', (self.cleaned,)), \
             patch.object(release_gate, 'REPORT_PATHS', self.local_reports()), \
             patch.object(publish_ready, 'ROOT', self.root):
            ready, reasons = publish_ready.ready_record(self.record, {
                'status': 'Accepted', 'submissionKind': 'PARTIAL_CODE', 'partialTemplateVerified': True,
                'assembledProgramSha256': hashlib.sha256(self.program.encode()).hexdigest(),
                'candidateSha256': hashlib.sha256(self.fragment.encode()).hexdigest(),
                'roundTrip': {'status': 'VISIBLE', 'sourceVisibleInSubmissionDetail': True, 'exactByteMatch': True}}, None)
        self.assertEqual(reasons, [])
        self.assertTrue(ready['completeCode'].endswith('.c'))
        self.assertTrue(ready['directlySubmittableCode'].endswith('.c'))

    def test_directly_downloaded_original_template_is_supported_without_alternate_api(self):
        asset = self.record['statement']['assets'][0]
        asset.pop('retrievedFrom')
        asset['sourceSha256'] = hashlib.sha256(self.template.encode()).hexdigest()
        self.helper().prepare_record(self.record, self.root)
        self.assertEqual((self.root / 'staging/repaired-code' / self.complete).read_text(), self.program)

    def test_direct_template_without_original_response_hash_is_not_trusted(self):
        self.record['statement']['assets'][0].pop('retrievedFrom')
        with self.assertRaises(ValueError):
            self.helper().prepare_record(self.record, self.root)

    def test_contextual_template_must_match_the_problem_eno(self):
        asset = self.record['statement']['assets'][0]
        asset['url'] = f'http://yoj.ruc.edu.cn/index.php/index/problem/downloadtk/pno/{self.number}/eno/99.html'
        self.record['problemUrl'] = f'http://yoj.ruc.edu.cn/index.php/index/problem/detail/pno/{self.number}/eno/99.html'
        self.helper().prepare_record(self.record, self.root)
        self.record['problemUrl'] = self.record['problemUrl'].replace('/eno/99', '/eno/98')
        with self.assertRaises(ValueError):
            self.helper().prepare_record(self.record, self.root)

    def test_matching_verified_assembly_is_not_submitted_again(self):
        previous = {'status': 'Accepted', 'candidateSha256': hashlib.sha256(self.fragment.encode()).hexdigest(),
                    'assembledProgramSha256': hashlib.sha256(self.program.encode()).hexdigest(),
                    'submissionKind': 'PARTIAL_CODE', 'partialTemplateVerified': True}
        result, records, _, submitted, requests = self.run_online(previous=previous)
        self.assertEqual(result, 0)
        self.assertEqual(records[self.number], previous)
        self.assertFalse(submitted)
        self.assertFalse(requests)

    def test_old_browser_fragment_ac_does_not_certify_a_new_assembly(self):
        previous = {'status': 'Accepted', 'manualBrowserEvidence': True,
                    'candidateSha256': hashlib.sha256(self.fragment.encode()).hexdigest()}
        result, records, _, submitted, _ = self.run_online(previous=previous)
        self.assertEqual(result, 0)
        self.assertEqual(len(submitted), 1)
        self.assertEqual(records[self.number]['assembledProgramSha256'], hashlib.sha256(self.program.encode()).hexdigest())

    def test_template_provenance_must_be_the_actual_yoj_read_api(self):
        self.record['statement']['assets'][0]['retrievedFrom']['url'] = 'https://other.example/api'
        with self.assertRaises(ValueError):
            self.helper().prepare_record(self.record, self.root)

    def test_complete_and_direct_paths_must_not_alias(self):
        self.record['archive']['completeCode'] = self.record['archive']['directlySubmittableCode']
        with self.assertRaisesRegex(ValueError, 'COLLISION'):
            self.helper().prepare_record(self.record, self.root)

    def test_unsupported_language_is_not_assembled(self):
        self.record['language'] = 'python3'
        with self.assertRaisesRegex(ValueError, 'UNSUPPORTED_LANGUAGE'):
            self.helper().prepare_record(self.record, self.root)

    def test_raw_complete_program_can_supply_one_exact_template_block(self):
        (self.raw / self.direct).write_text(self.program)
        (self.raw / self.complete).write_text(self.program)
        self.helper().prepare_record(self.record, self.root)
        self.assertEqual((self.root / 'staging/repaired-code' / self.direct).read_text(), self.fragment)
        self.assertEqual((self.raw / self.direct).read_text(), self.program)

    def test_duplicate_template_evidence_is_ambiguous(self):
        self.record['statement']['assets'] *= 2
        with self.assertRaisesRegex(ValueError, 'AMBIGUOUS'):
            self.helper().prepare_record(self.record, self.root)

    def test_template_problem_number_mismatch_fails_closed(self):
        self.record['statement']['assets'][0]['retrievedFrom']['problemNo'] = 2048
        with self.assertRaises(ValueError):
            self.helper().prepare_record(self.record, self.root)

    def test_frozen_release_is_not_reassembled(self):
        self.record['public']['status'] = 'PUBLIC_READY'
        _, report = self.run_assembler([self.record])
        self.assertEqual(report['records'], [])
        self.assertFalse((self.root / 'staging/repaired-code' / self.complete).exists())

    def test_comment_string_or_prototype_main_cannot_hide_an_untyped_fragment(self):
        self.record['statement']['assets'] = []
        for prefix in ('// int main() {}\n', '/* int main() {} */\n',
                       'const char *label = "int main() {}";\n', 'int main();\n',
                       'const char *label = R"x(int main() {})x";\n',
                       '#if 0\nint main() {}\n#endif\n', '#define ENTRY int main() {}\n',
                       '// continued comment \\\nint main() {}\n', '#define main helper\nint main() {}\n'):
            with self.subTest(prefix=prefix):
                (self.raw / self.direct).write_text(prefix + self.fragment)
                self.assertEqual(validate_candidates.validate_record(self.record)['category'], 'FILL_IN_FRAGMENT')

    def test_crlf_raw_fragment_keeps_its_bytes_but_matches_lf_sanitization(self):
        original = self.fragment.replace('\n', '\r\n').encode()
        for name in (self.complete, self.direct):
            (self.raw / name).write_bytes(original)
        self.helper().prepare_record(self.record, self.root)
        self.assertTrue(self.helper().is_assembled_candidate(self.record, self.cleaned / self.complete, self.root))
        self.assertEqual((self.raw / self.direct).read_bytes(), original)

    def test_current_raw_source_must_match_the_template_not_just_old_cleaned_files(self):
        (self.raw / self.direct).write_text('int main() { return 7; }\n')
        (self.raw / self.complete).write_text('int main() { return 7; }\n')
        _, report = self.run_assembler([self.record])
        self.assertEqual(report['records'][0]['status'], 'BLOCKED')
        self.assertFalse(self.helper().is_assembled_candidate(self.record, self.cleaned / self.complete, self.root))
        self.assertEqual(validate_candidates.validate_record(self.record)['category'], 'FILL_IN_FRAGMENT')

    def test_changed_raw_complete_cannot_reuse_a_different_valid_direct_fragment(self):
        (self.raw / self.complete).write_text('int main() { return 7; }\n')
        with self.assertRaises(ValueError):
            self.helper().prepare_record(self.record, self.root)
        self.assertFalse(self.helper().is_assembled_candidate(self.record, self.cleaned / self.complete, self.root))

    def test_output_directory_symlink_cannot_overwrite_raw_archive(self):
        repaired = self.root / 'staging/repaired-code'
        repaired.mkdir(parents=True)
        (repaired / self.folder).symlink_to(self.raw / self.folder, target_is_directory=True)
        original = [(self.raw / name).read_bytes() for name in (self.complete, self.direct)]
        with self.assertRaisesRegex(ValueError, 'PATH'):
            self.helper().prepare_record(self.record, self.root)
        self.assertEqual([(self.raw / name).read_bytes() for name in (self.complete, self.direct)], original)

    def test_old_report_temporary_symlink_cannot_overwrite_raw_archive(self):
        path = self.root / 'staging/template-assembly-report.json.tmp'
        path.symlink_to(self.raw / self.direct)
        original = (self.raw / self.direct).read_bytes()
        self.run_assembler([])
        self.assertEqual((self.raw / self.direct).read_bytes(), original)

    def test_report_destination_symlink_is_rejected_before_loading_or_writing(self):
        path = self.root / 'staging/template-assembly-report.json'
        path.symlink_to(self.raw / self.direct)
        original = (self.raw / self.direct).read_bytes()
        with self.assertRaisesRegex(ValueError, 'FILL_IN_.*PATH'):
            self.run_assembler([])
        self.assertEqual((self.raw / self.direct).read_bytes(), original)

    def test_current_blocked_result_cannot_reuse_old_candidates_or_be_cleared_by_another_selection(self):
        repaired = self.root / 'staging/repaired-code'
        repaired.mkdir(parents=True)
        link = repaired / self.folder
        link.symlink_to(self.raw / self.folder, target_is_directory=True)
        _, report = self.run_assembler([self.record])
        self.assertEqual(report['records'][0]['status'], 'BLOCKED')
        link.unlink()
        self.assertFalse(self.helper().is_assembled_candidate(self.record, self.cleaned / self.complete, self.root))
        self.run_assembler([])
        self.assertFalse(self.helper().is_assembled_candidate(self.record, self.cleaned / self.complete, self.root))
        self.run_assembler([self.record])
        self.assertTrue(self.helper().is_assembled_candidate(self.record, self.cleaned / self.complete, self.root))

    def test_new_single_slot_runs_assembly_then_sanitization_and_real_local_gates(self):
        original = '/* fixture-private-comment */' + self.fragment
        for name in (self.complete, self.direct):
            (self.raw / name).write_text(original)
        self.run_assembler([self.record])
        self.stack.enter_context(patch.object(sanitize_candidates, 'PERSONAL_MARKERS',
                                            (re.compile('fixture-private-comment'),)))
        with ExitStack() as patches:
            for name, value in {
                'ROOT': self.root, 'RAW_ROOT': self.raw, 'DATA_PATH': self.root / 'data/problems.json',
                'REPAIRED_ROOT': self.root / 'staging/repaired-code',
                'PORTABLE_ROOT': self.root / 'staging/cpp17-portable',
                'NORMALIZED_ROOT': self.root / 'staging/cpp17-compatible',
                'OUTPUT_ROOT': self.cleaned, 'REPORT_PATH': self.root / 'sanitize.json',
            }.items():
                patches.enter_context(patch.object(sanitize_candidates, name, value))
            patches.enter_context(patch.object(sys, 'argv', ['sanitize']))
            self.assertEqual(sanitize_candidates.main(), 0)
        self.assertEqual((self.cleaned / self.complete).read_text(), self.program)
        self.assertEqual((self.cleaned / self.direct).read_text(), self.fragment)
        self.assertEqual(validate_candidates.validate_record(self.record)['category'], 'PASS')
        self.test_real_template_assembled_candidate_compiles_and_links()
        self.assertEqual(run_samples.test_record(self.record, run=True)['category'], 'PASS')
        for name in (self.complete, self.direct):
            self.assertEqual((self.raw / name).read_text(), original)


if __name__ == '__main__':
    unittest.main()
