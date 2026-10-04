"""Verified YOJ HTTP transport; campus/VPN redirects are not judge responses.

No VPN login, credential export, TLS bypass, or retry of a submission lives here.
"""

from __future__ import annotations

import os
import ssl
import sys
from pathlib import Path
from urllib.parse import urlparse
from urllib.request import HTTPRedirectHandler, HTTPSHandler, build_opener


class YoJNetworkError(OSError):
    """Fixed, safe diagnostics: never include the redirected URL or headers."""


def validate_yoj_url(url: str) -> None:
    target = urlparse(url)
    if target.hostname == "wvpn.ruc.edu.cn":
        raise YoJNetworkError("YOJ_CAMPUS_ACCESS_REQUIRED: connect campus network/VPN; YOJ Keychain credentials cannot sign in to WebVPN")
    if (target.scheme not in {"http", "https"} or target.hostname != "yoj.ruc.edu.cn"
            or target.username is not None or target.password is not None
            or target.port not in {None, 80 if target.scheme == "http" else 443}):
        raise YoJNetworkError("YOJ_UNEXPECTED_DESTINATION: refused non-YOJ response")


class YoJRedirectHandler(HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        # Check before urllib contacts another origin or follows its TLS chain.
        # This also prevents forwarding account headers to a campus login page.
        try:
            validate_yoj_url(newurl)
            if urlparse(req.full_url).scheme == "https" and urlparse(newurl).scheme != "https":
                raise YoJNetworkError("YOJ_HTTPS_DOWNGRADE_REFUSED")
        except (OSError, ValueError):
            if fp is not None:
                fp.close()
            raise
        return super().redirect_request(req, fp, code, msg, headers, newurl)


def verified_context() -> ssl.SSLContext:
    context = ssl.create_default_context()
    # Framework Python on macOS can lack its bundled roots. Add the system
    # public CA bundle only in that case; respect explicit trust configuration.
    system_roots = Path("/etc/ssl/cert.pem")
    if (sys.platform == "darwin" and ssl.get_default_verify_paths().cafile is None
            and not os.environ.get("SSL_CERT_FILE") and not os.environ.get("SSL_CERT_DIR")
            and system_roots.is_file()):
        context.load_verify_locations(cafile=str(system_roots))
    return context


def build_yoj_opener(*handlers):
    return build_opener(YoJRedirectHandler(), HTTPSHandler(context=verified_context()), *handlers)
