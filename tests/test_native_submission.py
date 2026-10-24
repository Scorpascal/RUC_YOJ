import contextlib
import copy
import io
import json
import unittest
from unittest.mock import patch

from tools import audit_consistency as audit


class NativeSubmissionEvidenceTests(unittest.TestCase):
    def test_matching_but_forged_browser_projections_cannot_override_release_evidence(self):
        original = audit.load
        for field, value in [('codeSha256', '0' * 64),
                             ('codePath', '题解/another/code.cpp'),
                             ('submitEndpoint', 'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit_tk.html')]:
            def load(path):
                data = copy.deepcopy(original(path))
                if path in ['data/quick-submit.json', 'docs/data/quick-submit.json', 'docs/data/catalog.json']:
                    row = next(r for r in data['entries'] if int(r['problemNo']) == 285)
                    row['nativeSubmission'][field] = value
                return data
            output = io.StringIO()
            with patch.object(audit, 'load', side_effect=load), contextlib.redirect_stdout(output):
                result = audit.main()
            self.assertEqual(result, 1, field)
            self.assertTrue(any('native' in failure for failure in json.loads(output.getvalue())['failures']))
