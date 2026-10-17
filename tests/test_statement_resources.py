"""Resource recovery must not accept login/error pages or alter AC evidence."""
import hashlib
import json
import ssl
import tempfile
import unittest
from email.message import Message
from pathlib import Path
from unittest.mock import patch
from urllib.error import HTTPError

from tools import build_initial as builder


class Response:
    def __init__(self, body, content_type):
        self.body = body
        self.headers = Message()
        self.headers['Content-Type'] = content_type

    def __enter__(self):
        return self

    def __exit__(self, *_):
        pass

    def read(self, limit=-1):
        return self.body[:limit] if limit >= 0 else self.body


class StatementResourceTests(unittest.TestCase):
    image_url = 'https://images.example.org/diagram.svg'
    template_url = 'http://yoj.ruc.edu.cn/index.php/index/problem/downloadtk/pno/285.html'
    api_url = 'http://yoj.ruc.edu.cn/index.php/index/problem/gettkcode.html'
    template = '#include <iostream>\nint main() { ____qcodep____ }\n'

    def converter(self, root, **kwargs):
        return builder.StatementConverter('http://yoj.ruc.edu.cn/index.php/index/problem/detail/pno/285.html',
            asset_dir=root, download_assets=True, asset_extension='.cpp', **kwargs)

    def test_https_image_uses_verified_trust_context_and_preserves_bytes(self):
        svg = b'<svg xmlns="http://www.w3.org/2000/svg"><path d="M0 0L1 1"/></svg>'
        def transport(request, timeout, context=None):
            if context is None:
                raise ssl.SSLCertVerificationError('missing roots')
            self.assertTrue(context.check_hostname)
            self.assertEqual(context.verify_mode, ssl.CERT_REQUIRED)
            self.assertNotIn('Cookie', request.headers)
            self.assertNotIn('Authorization', request.headers)
            return Response(svg, 'image/svg+xml')
        with tempfile.TemporaryDirectory() as temporary, patch.object(builder, 'urlopen', transport):
            root = Path(temporary)
            item = self.converter(root).materialize_image(self.image_url, 'diagram')
            self.assertEqual(item['status'], 'DOWNLOADED')
            self.assertEqual((root / item['filename']).read_bytes(), svg)

    def test_https_attachment_also_uses_verified_trust_context(self):
        def transport(request, timeout, context=None):
            if context is None:
                raise ssl.SSLCertVerificationError('missing roots')
            self.assertTrue(context.check_hostname)
            self.assertEqual(context.verify_mode, ssl.CERT_REQUIRED)
            return Response(b'ZIP contents', 'application/zip')
        with tempfile.TemporaryDirectory() as temporary, patch.object(builder, 'urlopen', transport):
            item = self.converter(Path(temporary)).materialize_asset('https://files.example.org/sample.zip', 'sample')
            self.assertEqual(item['status'], 'DOWNLOADED')

    def test_failed_legacy_download_recovers_actual_public_template(self):
        def transport(request, timeout, **kwargs):
            if request.full_url == self.template_url:
                return Response(b'\xef\xbb\xbf', 'text/html')
            self.assertEqual(request.full_url, self.api_url)
            self.assertEqual(request.get_method(), 'POST')
            self.assertEqual(request.data, b'pno=285')
            self.assertNotIn('Cookie', request.headers)
            self.assertNotIn('Authorization', request.headers)
            return Response(('\ufeff' + json.dumps({'status': 1, 'content': self.template, 'content_arr': []})).encode(), 'application/json')
        with tempfile.TemporaryDirectory() as temporary, patch.object(builder, 'urlopen', transport):
            root = Path(temporary)
            item = self.converter(root).materialize_asset(self.template_url, 'template')
            self.assertEqual(item['status'], 'DOWNLOADED')
            self.assertEqual((root / item['filename']).read_text(), self.template)
            self.assertEqual(item['retrievedFrom'], {'url': self.api_url, 'method': 'POST', 'problemNo': 285, 'kind': 'YOJ_PUBLIC_TEMPLATE_API'})
            self.assertEqual(item['sourceSha256'], hashlib.sha256(self.template.encode()).hexdigest())

    def test_cache_keeps_verified_alternate_source_provenance(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path = root / '01_285.cpp'
            path.write_text(self.template)
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            provenance = {'url': self.api_url, 'method': 'POST', 'problemNo': 285, 'kind': 'YOJ_PUBLIC_TEMPLATE_API'}
            previous = {'url': self.template_url, 'filename': path.name, 'relativePath': 'assets/' + path.name,
                'status': 'DOWNLOADED', 'bytes': path.stat().st_size, 'sha256': digest, 'sourceSha256': digest,
                'transform': '', 'error': '', 'retrievedFrom': provenance}
            with patch.object(builder, 'urlopen', side_effect=AssertionError('cache must not contact network')):
                item = self.converter(root, previous_assets=[previous]).materialize_asset(self.template_url, 'template')
            self.assertEqual(item['status'], 'CACHED')
            self.assertEqual(item.get('retrievedFrom'), provenance)
            self.assertEqual(item['sourceSha256'], digest)

    def test_bom_prefixed_html_is_not_saved_as_image(self):
        with tempfile.TemporaryDirectory() as temporary, patch.object(builder, 'urlopen', return_value=Response(b'\xef\xbb\xbf<html>login</html>', 'application/octet-stream')):
            root = Path(temporary)
            item = self.converter(root).materialize_image(self.image_url, 'diagram')
            self.assertEqual(item['status'], 'DOWNLOAD_FAILED')
            self.assertFalse((root / item['filename']).exists())

    def test_empty_image_is_not_recorded_as_downloaded(self):
        with tempfile.TemporaryDirectory() as temporary, patch.object(builder, 'urlopen', return_value=Response(b'', 'image/svg+xml')):
            item = self.converter(Path(temporary)).materialize_image(self.image_url, 'diagram')
            self.assertEqual(item['status'], 'DOWNLOAD_FAILED')

    def test_missing_images_retain_original_link_and_failure_evidence(self):
        error = HTTPError(self.image_url, 404, 'Not found', None, None)
        with tempfile.TemporaryDirectory() as temporary, patch.object(builder, 'urlopen', side_effect=error):
            converter = self.converter(Path(temporary))
            item = converter.materialize_image(self.image_url, 'diagram')
            self.assertEqual(item['status'], 'DOWNLOAD_FAILED')
            self.assertEqual(item['url'], self.image_url)
            self.assertTrue(converter.warnings)

    def test_external_download_url_never_uses_yoj_template_api(self):
        with tempfile.TemporaryDirectory() as temporary, patch.object(builder, 'urlopen', return_value=Response(b'<html>error</html>', 'text/html')) as transport:
            item = self.converter(Path(temporary)).materialize_asset(self.template_url.replace('yoj.ruc.edu.cn', 'other.example.org'), 'template')
            self.assertEqual(item['status'], 'DOWNLOAD_FAILED')
            self.assertEqual(transport.call_count, 1)

    def test_invalid_public_template_does_not_become_an_attachment(self):
        def transport(request, timeout, **kwargs):
            if request.full_url == self.template_url:
                return Response(b'<html>error</html>', 'text/html')
            return Response(json.dumps({'status': 1, 'content': '<html>login</html>', 'content_arr': []}).encode(), 'application/json')
        with tempfile.TemporaryDirectory() as temporary, patch.object(builder, 'urlopen', transport):
            root = Path(temporary)
            item = self.converter(root).materialize_asset(self.template_url, 'template')
            self.assertEqual(item['status'], 'DOWNLOAD_FAILED')
            self.assertFalse((root / item['filename']).exists())

    def test_bom_html_inside_json_content_is_not_a_template(self):
        def transport(request, timeout, **kwargs):
            if request.full_url == self.template_url:
                return Response(b'<html>error</html>', 'text/html')
            return Response(json.dumps({'status': 1, 'content': '\ufeff<html>____qcodep____</html>'}).encode(), 'application/json')
        with tempfile.TemporaryDirectory() as temporary, patch.object(builder, 'urlopen', transport):
            root = Path(temporary)
            item = self.converter(root).materialize_asset(self.template_url, 'template')
            self.assertEqual(item['status'], 'DOWNLOAD_FAILED')
            self.assertFalse((root / item['filename']).exists())


if __name__ == '__main__':
    unittest.main()
