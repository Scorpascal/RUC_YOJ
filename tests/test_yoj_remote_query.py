"""Read-only deployment queries retain their exact-commit publication gate."""
import json
import os
import unittest
from types import SimpleNamespace
from urllib.error import URLError
from unittest.mock import patch
from tools import yoj_scheduler as scheduler


class RemoteQueryTests(unittest.TestCase):
    endpoint = 'https://api.github.com/repos/Scorpascal/RUC_YOJ/actions/runs?head_sha=' + 'a'*40 + '&per_page=20'

    def test_anonymous_limit_uses_existing_cli_with_get_and_no_yoj_environment(self):
        payload = {'workflow_runs': [{'head_sha': 'a'*40, 'path': scheduler.PAGES_WORKFLOW_PATH,
                                     'id': 1, 'status': 'completed', 'conclusion': 'success', 'html_url': 'https://github.com/Scorpascal/RUC_YOJ/actions/runs/1'}]}
        replies = [SimpleNamespace(returncode=22, stdout='', stderr='HTTP 403'),
                   SimpleNamespace(returncode=0, stdout=json.dumps(payload), stderr='')]
        with patch.object(scheduler.urllib.request, 'urlopen', side_effect=URLError('unavailable')), \
             patch.object(scheduler.shutil, 'which', side_effect=lambda name:'/usr/bin/'+name), \
             patch.object(scheduler, 'github_repository', return_value=('Scorpascal','RUC_YOJ')), \
             patch.object(scheduler.subprocess, 'run', side_effect=replies) as run, \
             patch.dict(os.environ, {'YOJ_LOGIN_PASS':'PRIVATE_MARKER','GH_TOKEN':'PRIVATE_MARKER'}):
            self.assertEqual(scheduler.github_api_json(self.endpoint), (payload,None))
        command = run.call_args.args[0]
        self.assertEqual(command[:2], ['/usr/bin/gh','api'])
        self.assertIn('GET',command)
        self.assertIn('github.com',command)
        self.assertEqual(command[-1], self.endpoint.removeprefix('https://api.github.com/'))
        self.assertEqual(run.call_args.kwargs['env']['GH_PROMPT_DISABLED'],'1')
        self.assertNotIn('PRIVATE_MARKER',str(run.call_args))

    def test_failed_or_unconfigured_cli_never_proves_deployment(self):
        for available in (False,True):
            with self.subTest(available=available), \
                 patch.object(scheduler.urllib.request,'urlopen',side_effect=URLError('unavailable')), \
                 patch.object(scheduler.shutil,'which',side_effect=lambda name:'/usr/bin/'+name if available or name=='curl' else None), \
                 patch.object(scheduler,'github_repository',return_value=('Scorpascal','RUC_YOJ')), \
                 patch.object(scheduler.subprocess,'run',return_value=SimpleNamespace(returncode=22,stdout='',stderr='unavailable')):
                payload,error=scheduler.github_api_json(self.endpoint)
            self.assertIsNone(payload)
            self.assertTrue(error)

    def test_cli_fallback_rejects_foreign_hosts_repositories_and_non_get_scopes(self):
        for endpoint in ('https://example.invalid/repos/Scorpascal/RUC_YOJ/actions/runs',
                         self.endpoint.replace('RUC_YOJ','other'),
                         self.endpoint.replace('/actions/runs?','/issues?'),
                         self.endpoint+'&private=PRIVATE_MARKER'):
            with self.subTest(endpoint=endpoint), \
                 patch.object(scheduler,'github_repository',return_value=('Scorpascal','RUC_YOJ')), \
                 patch.object(scheduler.shutil,'which',return_value='/usr/bin/gh'), \
                 patch.object(scheduler.subprocess,'run') as run:
                self.assertIsNone(scheduler.workflow_cli_json(endpoint))
            run.assert_not_called()

    def test_workflow_gate_still_requires_same_sha_path_and_success(self):
        for sha,path,result,expected in [('b'*40,scheduler.PAGES_WORKFLOW_PATH,'success','MISSING'),
                                         ('a'*40,'wrong.yml','success','MISSING'),
                                         ('a'*40,scheduler.PAGES_WORKFLOW_PATH,'failure','FAILURE'),
                                         ('a'*40,scheduler.PAGES_WORKFLOW_PATH,'success','SUCCESS')]:
            payload={'workflow_runs':[{'id':1,'head_sha':sha,'path':path,'status':'completed','conclusion':result,'html_url':'https://example.invalid/run'}]}
            with self.subTest(expected=expected), patch.object(scheduler,'github_repository',return_value=('Scorpascal','RUC_YOJ')), \
                 patch.object(scheduler,'github_api_json',return_value=(payload,None)):
                self.assertEqual(scheduler.remote_workflow_state('a'*40)[0],expected)


if __name__=='__main__':
    unittest.main()
