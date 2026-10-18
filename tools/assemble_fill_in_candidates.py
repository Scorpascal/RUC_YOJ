#!/usr/bin/env python3
"""Assemble advertised, hash-verified YOJ frameworks without editing raw ACs.

Only the two known single-slot legacy fragments are supported. Missing,
changed or ambiguous frameworks fail closed; this tool grants no publication
or online-verification status.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path
from urllib.parse import urlsplit


ROOT = Path(__file__).resolve().parents[1]
PROBLEMS = {285, 286}
PLACEHOLDER = '____qcodep____'


def bounded_path(root: Path, reference: str, parent: Path) -> Path:
    path = root / reference
    if not reference or path.is_symlink() or not path.resolve().is_relative_to(parent.resolve()):
        raise ValueError('FILL_IN_PATH_OUTSIDE_ARCHIVE')
    if not path.is_file() or path.stat().st_size > 1024 * 1024:
        raise ValueError('FILL_IN_RESOURCE_MISSING_OR_TOO_LARGE')
    return path


def verified_template(record: dict, root: Path) -> tuple[str, dict]:
    number = int(record['problemNo'])
    if number not in PROBLEMS:
        raise ValueError('FILL_IN_UNSUPPORTED_PROBLEM')
    statement = str((record.get('public') or {}).get('statement') or '')
    statement_path = bounded_path(root, statement, root / '题解')
    candidates = []
    for asset in (record.get('statement') or {}).get('assets') or []:
        url = urlsplit(str(asset.get('url') or ''))
        provenance = asset.get('retrievedFrom') or {}
        if (url.scheme not in {'http', 'https'} or url.netloc != 'yoj.ruc.edu.cn'
                or url.path != f'/index.php/index/problem/downloadtk/pno/{number}.html'
                or url.query or url.fragment or asset.get('status') not in {'DOWNLOADED', 'CACHED'}
                or provenance.get('kind') != 'YOJ_PUBLIC_TEMPLATE_API'
                or provenance.get('problemNo') != number):
            continue
        reference = str(asset.get('relativePath') or '')
        path = bounded_path(statement_path.parent, reference, statement_path.parent / 'assets')
        data = path.read_bytes()
        if hashlib.sha256(data).hexdigest() != asset.get('sha256'):
            raise ValueError('FILL_IN_TEMPLATE_HASH_MISMATCH')
        template = data.decode('utf-8')
        if template.count(PLACEHOLDER) != 1 or not re.search(r'\bmain\s*\(', template):
            raise ValueError('FILL_IN_TEMPLATE_NOT_SINGLE_SLOT_PROGRAM')
        candidates.append((template, asset))
    if len(candidates) != 1:
        raise ValueError('FILL_IN_TEMPLATE_MISSING_OR_AMBIGUOUS')
    return candidates[0]


def assemble(template: str, fragment: str) -> str:
    if not fragment.strip() or PLACEHOLDER in fragment or re.search(r'\bmain\s*\(', fragment):
        raise ValueError('FILL_IN_EXPECTED_FRAGMENT')
    return template.replace(PLACEHOLDER, fragment)


def prepare_record(record: dict, root: Path = ROOT) -> dict:
    template, asset = verified_template(record, root)
    archive = record.get('archive') or {}
    raw_root = root / '代码库'
    direct = bounded_path(root, str(archive.get('directlySubmittableCode') or ''), raw_root)
    complete = bounded_path(root, str(archive.get('completeCode') or ''), raw_root)
    fragment = direct.read_bytes().decode('utf-8')
    program = assemble(template, fragment)
    for source, content in ((complete, program), (direct, fragment)):
        target = root / 'staging/repaired-code' / source.relative_to(raw_root)
        target.parent.mkdir(parents=True, exist_ok=True)
        temporary = target.with_name(target.name + '.tmp')
        temporary.write_bytes(content.encode('utf-8'))
        temporary.replace(target)
    return {'problemNo': int(record['problemNo']), 'status': 'ASSEMBLED',
            'templateUrl': asset['url'], 'templateSha256': asset['sha256'],
            'rawFragmentSha256': hashlib.sha256(direct.read_bytes()).hexdigest(),
            'assembledSha256': hashlib.sha256(program.encode('utf-8')).hexdigest(),
            'rawArchiveChanged': False}


def is_assembled_candidate(record: dict, source: Path, root: Path = ROOT) -> bool:
    """Require the exact verified framework plus the selected cleaned fragment."""
    try:
        template, _ = verified_template(record, root)
        reference = str((record.get('archive') or {}).get('directlySubmittableCode') or '')
        relative = (root / reference).relative_to(root / '代码库')
        direct = bounded_path(root, 'staging/cleaned-code/' + relative.as_posix(), root / 'staging/cleaned-code')
        expected = assemble(template, direct.read_bytes().decode('utf-8')).encode('utf-8')
        return source.is_file() and source.read_bytes() == expected
    except (OSError, ValueError, KeyError, TypeError, UnicodeError):
        return False


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--problem', dest='problem_nos', type=int, action='append')
    args = parser.parse_args()
    selected = set(args.problem_nos or PROBLEMS) & PROBLEMS
    records = json.loads((ROOT / 'data/problems.json').read_text())['records']
    results = [prepare_record(row) for row in records if int(row['problemNo']) in selected]
    path = ROOT / 'staging/template-assembly-report.json'
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps({'schemaVersion': 1, 'rawArchiveChanged': False, 'records': results},
                               ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'assembled': [row['problemNo'] for row in results], 'rawArchiveChanged': False}))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
