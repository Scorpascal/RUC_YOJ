#!/usr/bin/env python3
"""Assemble advertised, hash-verified YOJ frameworks without editing raw ACs.

Single-slot C/C++ exercises are discovered from advertised template metadata,
partial-code routes, or archived fragments, never from a problem-number list.
Missing, changed or ambiguous frameworks fail closed; this tool grants no
publication or online-verification status.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import tempfile
from pathlib import Path
from urllib.parse import urlsplit

if __package__:
    from .yoj_context import problem_path
else:
    from yoj_context import problem_path


ROOT = Path(__file__).resolve().parents[1]
PLACEHOLDER = '____qcodep____'


def has_main_definition(source: str) -> bool:
    """Conservative lexical evidence, never a comment/string/prototype/call.

    Conditional or macro-generated entry points are deliberately unconfirmed;
    the normal compiler/linker gates still have to validate every candidate.
    """
    literals = (r'R"(?P<raw>[^ ()\\\t\r\n]{0,16})\(.*?\)(?P=raw)"'
                r'|"(?:\\.|[^"\\])*"'
                r"|(?<![\w])(?:u8|u|U|L)?'(?:\\.|[^'\\\r\n])*'"
                r'|//[^\n]*|/\*.*?\*/')
    # C/C++ splice escaped newlines before recognizing comments/directives.
    source = source.replace('\\\r\n', '').replace('\\\n', '')
    masked = re.sub(literals, lambda match: '\n' * match[0].count('\n') + ' ', source, flags=re.S)
    if re.search(r'^\s*#\s*(?:define|undef)\s+main\b', masked, flags=re.M):
        return False
    lines = []
    conditional_depth, continuation = 0, False
    for line in masked.splitlines(keepends=True):
        directive = re.match(r'\s*#\s*(\w+)', line)
        if directive or continuation:
            if directive and directive[1] in {'if', 'ifdef', 'ifndef'}:
                conditional_depth += 1
            elif directive and directive[1] == 'endif':
                conditional_depth = max(0, conditional_depth - 1)
            continuation = line.rstrip('\r\n').endswith('\\')
            continue
        if not conditional_depth:
            lines.append(line)
    masked = ''.join(lines)
    definition = r'\b(?:int|signed(?:\s+int)?|auto)\s+main\s*\([^(){};]*\)\s*(?:noexcept\s*)?(?:->\s*int\s*)?\{'
    depth = 0
    for token in re.finditer(definition + r'|[{}]', masked):
        value = token[0]
        if len(value) > 1:
            if depth == 0:
                return True
            depth += 1
        elif value == '{':
            depth += 1
        else:
            depth -= 1
    return False


def bounded_path(root: Path, reference: str, parent: Path) -> Path:
    path = root / reference
    if not reference or path.is_symlink() or not path.resolve().is_relative_to(parent.resolve()):
        raise ValueError('FILL_IN_PATH_OUTSIDE_ARCHIVE')
    if not path.is_file() or path.stat().st_size > 1024 * 1024:
        raise ValueError('FILL_IN_RESOURCE_MISSING_OR_TOO_LARGE')
    return path.resolve()


def requires_template_assembly(record: dict, root: Path = ROOT) -> bool:
    """Recognize intent even when the actual template is missing or untrusted.

    Detection is not proof: every detected exercise must still pass
    verified_template. An untyped C/C++ fragment also fails closed instead of
    letting a stale, previously repaired main masquerade as its framework.
    """
    if (record.get('submissionKind') == 'PARTIAL_CODE'
            or '/detailtk/' in str(record.get('problemUrl') or '')
            or any('/downloadtk/' in str(asset.get('url') or '')
                   or (asset.get('retrievedFrom') or {}).get('kind') == 'YOJ_PUBLIC_TEMPLATE_API'
                   for asset in (record.get('statement') or {}).get('assets') or [])):
        return True
    language = str(record.get('language') or '')
    if language == 'c' or language.startswith(('cpp', 'c-')):
        reference = str((record.get('archive') or {}).get('directlySubmittableCode') or '')
        if reference:
            try:
                source = bounded_path(root, reference, root / '代码库').read_text(encoding='utf-8')
                return not has_main_definition(source)
            except (OSError, ValueError, UnicodeError):
                return True
    return False


def verified_template(record: dict, root: Path) -> tuple[str, dict]:
    number = int(record['problemNo'])
    language = str(record.get('language') or '')
    if not (language == 'c' or language.startswith(('cpp', 'c-'))):
        raise ValueError('FILL_IN_UNSUPPORTED_LANGUAGE')
    statement = str((record.get('public') or {}).get('statement') or '')
    statement_path = bounded_path(root, statement, root / '题解')
    context = problem_path(number, record.get('problemUrl'))
    candidates = []
    for asset in (record.get('statement') or {}).get('assets') or []:
        url = urlsplit(str(asset.get('url') or ''))
        provenance = asset.get('retrievedFrom') or {}
        route = re.fullmatch(rf'/index\.php/index/problem/downloadtk/pno/{number}(?:/eno/([1-9]\d*))?\.html', url.path)
        if (url.scheme not in {'http', 'https'} or url.netloc != 'yoj.ruc.edu.cn' or not route
                or url.query or url.fragment or asset.get('status') not in {'DOWNLOADED', 'CACHED'}):
            continue
        if route[1] and not context.endswith(f'/eno/{route[1]}.html'):
            raise ValueError('FILL_IN_TEMPLATE_CONTEXT_MISMATCH')
        if provenance and (provenance.get('kind') != 'YOJ_PUBLIC_TEMPLATE_API'
                or provenance.get('problemNo') != number
                or provenance.get('method') != 'POST'
                or provenance.get('url') not in {
                    'http://yoj.ruc.edu.cn/index.php/index/problem/gettkcode.html',
                    'https://yoj.ruc.edu.cn/index.php/index/problem/gettkcode.html'}):
            continue
        if not provenance and (asset.get('sourceSha256') != asset.get('sha256') or asset.get('transform')):
            continue
        reference = str(asset.get('relativePath') or '')
        path = bounded_path(statement_path.parent, reference, statement_path.parent / 'assets')
        data = path.read_bytes()
        if hashlib.sha256(data).hexdigest() != asset.get('sha256'):
            raise ValueError('FILL_IN_TEMPLATE_HASH_MISMATCH')
        template = data.decode('utf-8')
        if template.count(PLACEHOLDER) != 1 or not has_main_definition(template):
            raise ValueError('FILL_IN_TEMPLATE_NOT_SINGLE_SLOT_PROGRAM')
        candidates.append((template, asset))
    if len(candidates) != 1:
        raise ValueError('FILL_IN_TEMPLATE_MISSING_OR_AMBIGUOUS')
    return candidates[0]


def assemble(template: str, fragment: str) -> str:
    if not fragment.strip() or PLACEHOLDER in fragment or has_main_definition(fragment):
        raise ValueError('FILL_IN_EXPECTED_FRAGMENT')
    return template.replace(PLACEHOLDER, fragment)


def raw_fragment(record: dict, template: str, root: Path) -> tuple[str, Path, Path]:
    """Bind both original archive files to the current, exact framework."""
    archive = record.get('archive') or {}
    raw_root = root / '代码库'
    direct = bounded_path(root, str(archive.get('directlySubmittableCode') or ''), raw_root)
    complete = bounded_path(root, str(archive.get('completeCode') or ''), raw_root)
    if direct == complete:
        raise ValueError('FILL_IN_SOURCE_PATH_COLLISION')
    fragment = direct.read_bytes().decode('utf-8')
    if has_main_definition(fragment):
        # Full-source archives are supported only when their fixed prefix and
        # suffix match exactly; do not infer blocks from whitespace or braces.
        prefix, suffix = template.split(PLACEHOLDER)
        if not fragment.startswith(prefix) or not fragment.endswith(suffix):
            raise ValueError('FILL_IN_FULL_SOURCE_TEMPLATE_MISMATCH')
        fragment = fragment[len(prefix):len(fragment) - len(suffix) if suffix else None]
    program = assemble(template, fragment)
    if complete.read_bytes().decode('utf-8') not in {fragment, program}:
        raise ValueError('FILL_IN_RAW_SOURCES_MISMATCH')
    return fragment, complete, direct


def prepare_record(record: dict, root: Path = ROOT) -> dict:
    template, asset = verified_template(record, root)
    fragment, complete, direct = raw_fragment(record, template, root)
    program = assemble(template, fragment)
    targets = []
    for source, content in ((complete, program), (direct, fragment)):
        target = root / 'staging/repaired-code' / source.relative_to((root / '代码库').resolve())
        expected = root.resolve() / target.relative_to(root)
        if target.is_symlink() or target.resolve() != expected:
            raise ValueError('FILL_IN_OUTPUT_PATH_OUTSIDE_STAGING')
        targets.append((target, content))
    for target, content in targets:
        target.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.NamedTemporaryFile(dir=target.parent, prefix=target.name + '.', suffix='.tmp', delete=False) as handle:
            handle.write(content.encode('utf-8'))
        Path(handle.name).replace(target)
    return {'problemNo': int(record['problemNo']), 'status': 'ASSEMBLED',
            'templateUrl': asset['url'], 'templateSha256': asset['sha256'],
            'rawFragmentSha256': hashlib.sha256(direct.read_bytes()).hexdigest(),
            'assembledSha256': hashlib.sha256(program.encode('utf-8')).hexdigest(),
            'rawArchiveChanged': False}


def is_assembled_candidate(record: dict, source: Path, root: Path = ROOT) -> bool:
    """Require the exact verified framework plus the selected cleaned fragment."""
    try:
        report = root / 'staging/template-assembly-report.json'
        if report.is_file() and any(
            str(row.get('problemNo')) == str(record['problemNo']) and row.get('status') == 'BLOCKED'
            for row in json.loads(report.read_text(encoding='utf-8')).get('records') or []
        ):
            return False
        template, _ = verified_template(record, root)
        fragment, _, _ = raw_fragment(record, template, root)
        if __package__:
            from .sanitize_candidates import sanitize_comments
        else:
            from sanitize_candidates import sanitize_comments
        # sanitize_candidates reads with universal newlines and writes LF.
        # Reproduce that transformation without touching the original bytes.
        cleaned_fragment, _ = sanitize_comments(fragment.replace('\r\n', '\n').replace('\r', '\n'), python_comment=False)
        reference = str((record.get('archive') or {}).get('directlySubmittableCode') or '')
        relative = (root / reference).relative_to(root / '代码库')
        direct = bounded_path(root, 'staging/cleaned-code/' + relative.as_posix(), root / 'staging/cleaned-code')
        if direct.read_bytes() != cleaned_fragment.encode('utf-8'):
            return False
        expected = assemble(template.replace('\r\n', '\n').replace('\r', '\n'), cleaned_fragment).encode('utf-8')
        return source.is_file() and source.read_bytes() == expected
    except (OSError, ValueError, KeyError, TypeError, UnicodeError):
        return False


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--problem', dest='problem_nos', type=int, action='append')
    args = parser.parse_args()
    selected = set(args.problem_nos or [])
    records = json.loads((ROOT / 'data/problems.json').read_text())['records']
    results = []
    for record in records:
        number = int(record['problemNo'])
        if selected and number not in selected:
            continue
        if (record.get('public') or {}).get('status') == 'PUBLIC_READY':
            continue
        if not (record.get('archive') or {}).get('completeCode') or not requires_template_assembly(record, ROOT):
            continue
        try:
            results.append(prepare_record(record, ROOT))
        except (OSError, ValueError, KeyError, TypeError, UnicodeError) as exc:
            reason = str(exc) if re.fullmatch(r'FILL_IN_[A-Z_]+', str(exc)) else 'FILL_IN_TEMPLATE_OR_SOURCE_UNREADABLE'
            results.append({'problemNo': number, 'status': 'BLOCKED', 'reason': reason})
    path = ROOT / 'staging/template-assembly-report.json'
    if path.is_symlink() or path.resolve() != ROOT.resolve() / 'staging/template-assembly-report.json':
        raise ValueError('FILL_IN_REPORT_PATH_OUTSIDE_STAGING')
    path.parent.mkdir(parents=True, exist_ok=True)
    previous = {}
    if path.is_file():
        previous = {int(row['problemNo']): row for row in json.loads(path.read_text(encoding='utf-8')).get('records') or []}
    previous.update({row['problemNo']: row for row in results})
    encoded = json.dumps({'schemaVersion': 1, 'rawArchiveChanged': False, 'records': list(previous.values())},
                         ensure_ascii=False, indent=2) + '\n'
    with tempfile.NamedTemporaryFile(dir=path.parent, prefix=path.name + '.', suffix='.tmp', delete=False) as handle:
        handle.write(encoded.encode('utf-8'))
    Path(handle.name).replace(path)
    print(json.dumps({'assembled': [row['problemNo'] for row in results if row['status'] == 'ASSEMBLED'],
                      'blocked': [row['problemNo'] for row in results if row['status'] == 'BLOCKED'],
                      'rawArchiveChanged': False}))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
