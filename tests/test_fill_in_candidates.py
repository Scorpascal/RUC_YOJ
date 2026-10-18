"""Real-template fragments must earn the same local and publication gates."""

import hashlib
import importlib
import importlib.util
import json
import sys
import tempfile
import unittest
from types import SimpleNamespace
from contextlib import ExitStack
from pathlib import Path
from unittest.mock import patch

from tools import build_initial, compile_candidates, online_verify, publish_ready, release_gate, run_samples, validate_candidates


class FillInTests(unittest.TestCase):
    template = '#include <stdio.h>\nint main() {\n____qcodep____\n}\n'
    fragment = 'printf("2\\n");\nreturn 0;\n'
    program = '#include <stdio.h>\nint main() {\nprintf("2\\n");\nreturn 0;\n\n}\n'

    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.raw = self.root / '代码库'
        self.cleaned = self.root / 'staging/cleaned-code'
        self.folder = '0285_fixture'
        self.complete = self.folder + '/0285_提交_1_cpp_完整代码.cpp'
        self.direct = self.folder + '/0285_提交_1_cpp_可提交代码.cpp'
        self.statement = self.root / '题解' / self.folder / '0285_题目.md'
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
            'problemNo': '285', 'title': 'Fixture', 'folder': self.folder, 'language': 'cpp',
            'archive': {'completeCode': '代码库/' + self.complete,
                        'directlySubmittableCode': '代码库/' + self.direct},
            'public': {'statement': '题解/' + self.folder + '/0285_题目.md'},
            'statement': {'assets': [{'url': 'http://yoj.ruc.edu.cn/index.php/index/problem/downloadtk/pno/285.html',
                'relativePath': 'assets/template.cpp', 'status': 'CACHED',
                'sha256': hashlib.sha256(self.template.encode()).hexdigest(),
                'retrievedFrom': {'kind': 'YOJ_PUBLIC_TEMPLATE_API', 'problemNo': 285}}]},
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
        code = self.statement.parent / '0285_填空提交片段.cpp'
        code.write_text(self.fragment)
        reference = code.relative_to(self.root).as_posix()
        ready = self.root / 'ready.json'
        ready.write_text(json.dumps({'records': [{'problemNo': 285, 'status': 'PUBLIC_READY',
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
        reports = {}
        for kind in ('validation', 'compile', 'samples'):
            path = self.root / f'{kind}.json'
            path.write_text(json.dumps({'results': [{'problemNo': '285', 'category': 'PASS',
                'candidateSha256': hashlib.sha256(self.program.encode()).hexdigest()}]}))
            reports[kind] = path
        sanitization = self.root / 'sanitize.json'
        sanitization.write_text(json.dumps({'fileRecords': [{'candidate': 'staging/cleaned-code/' + name,
            'candidateSha256': hashlib.sha256(content.encode()).hexdigest(),
            'remainingConfiguredMarkers': []} for name, content in [(self.complete, self.program), (self.direct, self.fragment)]]}))
        reports['sanitization'] = sanitization
        ready_path = self.root / 'ready.json'
        online = {'problemNo': 285, 'status': 'Accepted', 'submissionNo': 100,
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
            publish_ready.apply_release([self.record], {'285': ready}, 'fixture')
        self.assertEqual((self.raw / self.complete).read_text(), self.fragment)
        self.assertEqual((self.raw / self.direct).read_text(), self.fragment)
        self.assertEqual((self.root / ready['completeCode']).read_text(), self.program)
        self.assertEqual((self.root / ready['directlySubmittableCode']).read_text(), self.fragment)


if __name__ == '__main__':
    unittest.main()
