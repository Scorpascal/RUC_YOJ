"""Read-only account entry discovery and narrowly validated problem contexts.

Full account snapshots stay in .yoj-sync. No enrollment, roster, credential,
submission or guessed-ID endpoint is part of this graph.
"""
from collections import deque
from html.parser import HTMLParser
import re
from urllib.parse import parse_qsl, urlencode, urljoin, urlsplit

BASE = 'http://yoj.ruc.edu.cn'
PREFIX = '/index.php/index/'


class Links(HTMLParser):
    def __init__(self, page):
        super().__init__(convert_charrefs=True)
        self.urls = []
        self.seen_body = self.closed_body = self.closed_html = False
        self.table_depth = 0
        self.feed(page)

    def handle_starttag(self, tag, attrs):
        if tag.lower() == 'a':
            self.urls.extend(value for key, value in attrs if key == 'href' and value)
        if tag.lower() == 'body':
            self.seen_body = True
        if tag.lower() == 'table':
            self.table_depth += 1

    def handle_endtag(self, tag):
        if tag.lower() == 'body':
            self.closed_body = True
        if tag.lower() == 'html':
            self.closed_html = True
        if tag.lower() == 'table':
            self.table_depth -= 1

    @property
    def complete(self):
        return self.seen_body and self.closed_body and self.closed_html and self.table_depth == 0


def is_access_denied(page):
    """Only a platform access-error response, never a child/status label."""
    return bool('/login/logout' not in page
                and re.search(r'system-message|<title[^>]*>\s*跳转提示', page, re.I)
                and re.search(r'看不到|尚未开始|未报名|没有权限|无权访问|请先报名|请先加入', page))


def local_route(value):
    """Only ordinary same-origin paths; never tokens, queries or userinfo."""
    target = urlsplit(urljoin(BASE + '/', value))
    if (target.scheme not in {'http', 'https'} or target.hostname != 'yoj.ruc.edu.cn'
            or target.username is not None or target.password is not None
            or target.port not in {None, 80 if target.scheme == 'http' else 443}
            or target.query or target.fragment):
        raise ValueError('INVALID_YOJ_CONTEXT')
    return target.path


def problem_path(problem_no, value=None, *, partial=False):
    path = local_route(value or f'{PREFIX}problem/detail/pno/{problem_no}.html')
    match = re.fullmatch(r'/index\.php/(?:index/)?problem/(?:detail|detailtk)/pno/([1-9]\d*)(?:/eno/([1-9]\d*))?(?:\.html)?', path)
    if not match or int(match[1]) != int(problem_no):
        raise ValueError('INVALID_PROBLEM_CONTEXT')
    suffix = f'/eno/{match[2]}' if match[2] else ''
    kind = 'detailtk' if partial else 'detail'
    return f'{PREFIX}problem/{kind}/pno/{int(problem_no)}{suffix}.html'


def entry_path(value):
    try:
        target = urlsplit(urljoin(BASE + '/', value))
        query = parse_qsl(target.query, keep_blank_values=True)
        if (target.query and (len(query) != 1 or query[0][0] not in {'p', 'page'}
                             or not re.fullmatch(r'[1-9]\d*', query[0][1]))):
            return None
        path = local_route(target._replace(query='').geturl())
    except (ValueError, TypeError):
        return None
    match = re.fullmatch(r'/index\.php/(?:index/)?(course|contest|homework)/(index|detail)((?:/(?:cno/[A-Za-z0-9]+|eno/[1-9]\d*|hno/[1-9]\d*|p/[1-9]\d*|page/[1-9]\d*))*)(?:\.html)?', path)
    if not match:
        return None
    surface, kind, suffix = match.groups()
    keys = re.findall(r'/(cno|eno|hno|p|page)/', suffix)
    if len(keys) != len(set(keys)) or ('p' in keys and 'page' in keys):
        return None
    if query and set(keys) & {'p', 'page'}:
        return None
    allowed = {'course': {'cno', 'p', 'page'}, 'contest': {'cno', 'eno', 'p', 'page'}, 'homework': {'cno', 'hno', 'p', 'page'}}
    if set(keys) - allowed[surface]:
        return None
    if kind == 'detail' and {'course': 'cno', 'contest': 'eno', 'homework': 'hno'}[surface] not in keys:
        return None
    return f'{PREFIX}{surface}/{kind}{suffix}.html' + ('?' + urlencode(query) if query else '')


def collect_problem_paths(page, paths, source_path):
    links = Links(page)
    if not links.complete:
        raise RuntimeError('ACCOUNT_PROBLEM_INDEX_INCOMPLETE')
    for value in links.urls:
        try:
            normalized = local_route(urljoin(urljoin(BASE, source_path), value))
        except (ValueError, TypeError):
            continue
        match = re.fullmatch(r'/index\.php/(?:index/)?problem/detail/pno/([1-9]\d*)(?:/eno/[1-9]\d*)?(?:\.html)?', normalized)
        if match:
            number = match[1]
            context = problem_path(int(number), normalized)
            if context not in paths.setdefault(number, []):
                paths[number].append(context)


def discover_account_contexts(client, limiter, numbers, max_pages, initial_paths=None):
    """Follow only links actually advertised by accessible account pages."""
    paths = {str(n): list((initial_paths or {}).get(str(n)) or [problem_path(n)]) for n in numbers}
    queue = deque([PREFIX + 'course/index.html', PREFIX + 'contest/index.html'])
    queued = set(queue)
    counts = {'entryPages': 0, 'courses': 0, 'exams': 0, 'homework': 0, 'inaccessibleEntries': 0}
    while queue:
        if counts['entryPages'] >= max_pages:
            raise RuntimeError('ACCOUNT_ENTRY_PAGE_LIMIT')
        path = queue.popleft()
        limiter.before_request()
        page = client.request(path)
        counts['entryPages'] += 1
        # A server-declared inaccessible detail is not an account-visible
        # entry. An unrecognizable/truncated response is NOT an empty list.
        if is_access_denied(page):
            if '/detail/' not in path:
                raise RuntimeError('ACCOUNT_ENTRY_ACCESS_REQUIRED')
            counts['inaccessibleEntries'] += 1
            continue
        links = Links(page)
        if (not links.complete or '/login/logout' not in page or not re.search(r'<(?:table|h[1-4])\b|class=[\"\'][^\"\']*\b(?:cards|main container)\b', page, re.I)):
            raise RuntimeError('ACCOUNT_ENTRY_RESPONSE_INVALID')
        if '/detail/' in path:
            counts[{'course': 'courses', 'contest': 'exams', 'homework': 'homework'}[path.split('/')[3]]] += 1
        for value in links.urls:
            absolute = urljoin(BASE + path, value)
            candidate = entry_path(absolute)
            if candidate:
                if candidate not in queued:
                    queued.add(candidate)
                    queue.append(candidate)
                continue
            try:
                normalized = local_route(absolute)
            except (ValueError, TypeError):
                continue
            match = re.fullmatch(r'/index\.php/(?:index/)?problem/detail/pno/([1-9]\d*)(?:/eno/[1-9]\d*)?(?:\.html)?', normalized)
            if match:
                number = match[1]
                context = problem_path(int(number), normalized)
                if context not in paths.setdefault(number, []):
                    paths[number].append(context)
                continue
    return paths, counts
