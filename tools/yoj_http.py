"""Verified YOJ HTTP transport; campus/VPN redirects are not judge responses.

No VPN login, credential export, TLS bypass, or retry of a submission lives here.
"""

from __future__ import annotations

import errno
import os
import socket
import ssl
import sys
import time
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.parse import urlparse
from urllib.request import HTTPRedirectHandler, HTTPSHandler, build_opener


REASONS = frozenset({
    "NONE", "YOJ_TLS_CERTIFICATE_ERROR", "YOJ_CAMPUS_ACCESS_REQUIRED",
    "YOJ_AUTH_REQUIRED", "YOJ_TRANSIENT_NETWORK_ERROR", "YOJ_UNEXPECTED_DESTINATION",
    "YOJ_HTTPS_DOWNGRADE_REFUSED", "YOJ_INVALID_PUBLIC_INDEX", "YOJ_SOURCE_ERROR",
})


class YoJNetworkError(OSError):
    """Fixed, safe diagnostics: never include the redirected URL or headers."""

    def __init__(self, reason: str):
        # Accept the old fixed-prefix diagnostics without retaining their text.
        category = reason.partition(":")[0]
        self.reason = category if category in REASONS and category != "NONE" else "YOJ_SOURCE_ERROR"
        super().__init__(self.reason)


def classify_yoj_error(error: BaseException) -> str:
    """Classify network evidence without serializing URLs or exception text.

    An arbitrary URLError/OSError is not proof of a temporary failure. In
    particular, a permanent DNS miss and TLS protocol errors are not retried.
    """

    if isinstance(error, YoJNetworkError):
        return error.reason
    if isinstance(error, HTTPError):
        if error.code in {401, 403}:
            return "YOJ_AUTH_REQUIRED"
        if error.code == 429 or 500 <= error.code <= 599:
            return "YOJ_TRANSIENT_NETWORK_ERROR"
        return "YOJ_SOURCE_ERROR"
    if isinstance(error, URLError) and isinstance(error.reason, BaseException):
        return classify_yoj_error(error.reason)
    if isinstance(error, ssl.SSLCertVerificationError):
        return "YOJ_TLS_CERTIFICATE_ERROR"
    if isinstance(error, ssl.SSLError):
        if getattr(error, "reason", None) == "CERTIFICATE_VERIFY_FAILED":
            return "YOJ_TLS_CERTIFICATE_ERROR"
        return "YOJ_SOURCE_ERROR"
    if isinstance(error, socket.gaierror):
        return "YOJ_TRANSIENT_NETWORK_ERROR" if error.errno == socket.EAI_AGAIN else "YOJ_SOURCE_ERROR"
    if isinstance(error, (TimeoutError, ConnectionError)):
        return "YOJ_TRANSIENT_NETWORK_ERROR"
    if isinstance(error, OSError) and error.errno in {
        errno.ETIMEDOUT, errno.ECONNRESET, errno.ECONNREFUSED, errno.ECONNABORTED,
        errno.ENETUNREACH, errno.EHOSTUNREACH, errno.EPIPE,
    }:
        return "YOJ_TRANSIENT_NETWORK_ERROR"
    return "YOJ_SOURCE_ERROR"


def source_state_for_reason(reason: str) -> str:
    if reason == "NONE":
        return "AVAILABLE"
    if reason in {"YOJ_TLS_CERTIFICATE_ERROR", "YOJ_CAMPUS_ACCESS_REQUIRED", "YOJ_AUTH_REQUIRED"}:
        return "BLOCKED"
    if reason == "YOJ_TRANSIENT_NETWORK_ERROR":
        return "TRANSIENT_ERROR"
    return "ERROR"


def read_public_get(opener, request, timeout: float) -> tuple[bytes, str, str]:
    """Read only an idempotent public GET, with at most three short attempts.

    Authenticated requests and submissions keep their existing caller policy.
    Reads are safe to repeat; authenticated requests and POSTs never use this
    helper. Decoding and content validation are left to the public-index audit.
    """

    if request.get_method() != "GET" or request.data is not None:
        raise ValueError("public retry transport requires a GET without a body")
    validate_yoj_url(request.full_url)
    for attempt in range(1, 4):
        try:
            with opener.open(request, timeout=timeout) as response:
                final_url = response.geturl()
                validate_yoj_url(final_url)
                if urlparse(request.full_url).scheme == "https" and urlparse(final_url).scheme != "https":
                    raise YoJNetworkError("YOJ_HTTPS_DOWNGRADE_REFUSED")
                body = response.read()
                charset = response.headers.get_content_charset() or "utf-8"
                return body, charset, final_url
        except OSError as exc:
            reason = classify_yoj_error(exc)
            if isinstance(exc, HTTPError):
                exc.close()
            if reason != "YOJ_TRANSIENT_NETWORK_ERROR" or attempt == 3:
                raise YoJNetworkError(reason) from None
            time.sleep(2 * attempt)


def validate_yoj_url(url: str) -> None:
    try:
        target = urlparse(url)
        if target.hostname in {"wvpn.ruc.edu.cn", "webvpn.ruc.edu.cn"}:
            raise YoJNetworkError("YOJ_CAMPUS_ACCESS_REQUIRED")
        if (target.scheme not in {"http", "https"} or target.hostname != "yoj.ruc.edu.cn"
                or target.username is not None or target.password is not None
                or target.port not in {None, 80 if target.scheme == "http" else 443}):
            raise YoJNetworkError("YOJ_UNEXPECTED_DESTINATION")
    except ValueError:
        raise YoJNetworkError("YOJ_UNEXPECTED_DESTINATION") from None


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
