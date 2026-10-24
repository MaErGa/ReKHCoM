"""Import verified historical screenshots without rewriting reviews or creating duplicate families.

Default is a read-only plan. Source files stay outside git; the manifest records their hashes.
"""
import argparse
import hashlib
import io
import json
import sqlite3
from contextlib import closing
from pathlib import Path

from PIL import Image
from server import ROOT, Store, now


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def assets_readonly(data):
    database = Path(data) / 'revisiones.sqlite3'
    if not database.exists():
        return []
    with closing(sqlite3.connect(database.resolve().as_uri() + '?mode=ro', uri=True)) as db:
        return [json.loads(row[0]) for row in db.execute('SELECT data FROM assets ORDER BY rowid')]


def pixel_digest(path):
    with Image.open(path) as im:
        im.load()
        out = io.BytesIO()
        im.convert('RGBA').save(out, format='PNG')
        return hashlib.sha256(out.getvalue()).hexdigest()


def plan(manifest, workspace, data):
    if manifest.get('schema') != 'rekhcom.art-history/1':
        raise ValueError('Unsupported manifest schema')
    workspace = Path(workspace).resolve()
    assets = assets_readonly(data)
    keys = set()
    result = []
    for entry in manifest['entries']:
        key = entry['key']
        if key in keys:
            raise ValueError('Duplicate manifest key: ' + key)
        keys.add(key)
        known = [a for a in assets if a.get('revision_info', {}).get('catalog_key') == key
                 or a['id'] in entry.get('existing_family_ids', [])]
        if known:
            result.append(dict(key=key, action='preserved', id=known[-1]['id'], status=known[-1]['status']))
            continue
        source = workspace / entry['source']
        evidence = entry.get('evidence_files', [])
        if not evidence:
            raise ValueError('Historical screenshot needs provenance files: ' + key)
        absent = [str(p) for p in [source] + [workspace / x['path'] for x in evidence] if not p.is_file()]
        if absent:
            result.append(dict(key=key, action='missing', paths=absent))
            continue
        if digest(source) != entry['source_sha256']:
            raise ValueError('Source hash mismatch: ' + key)
        for file in evidence:
            if digest(workspace / file['path']) != file['sha256']:
                raise ValueError('Provenance hash mismatch: ' + key + ': ' + file['path'])
        status = entry.get('status', 'pending')
        if status not in ('pending', 'approved', 'honors'):
            raise ValueError('Invalid historical status: ' + key)
        approval = entry.get('approval_record')
        if status != 'pending':
            if not approval or approval.get('source_sha256') != entry['source_sha256']:
                raise ValueError('Approval must identify the exact screenshot: ' + key)
            if not approval.get('quote') or not any(x['path'] == approval.get('document') for x in evidence):
                raise ValueError('Approval needs quoted, hashed documentary evidence: ' + key)
            if approval['quote'] not in (workspace / approval['document']).read_text(encoding='utf-8'):
                raise ValueError('Approval quote is absent from its document: ' + key)
        pixel_sha = pixel_digest(source)
        same = [a for a in assets if a['sha256'] == pixel_sha]
        if same:
            result.append(dict(key=key, action='already_catalogued', id=same[-1]['id'], status=same[-1]['status']))
        else:
            result.append(dict(key=key, action='import', source=str(source), status=status))
    return result


def run(manifest, workspace, data, apply=False):
    rows = plan(manifest, workspace, data)  # Validate the entire plan before the first write.
    if apply:
        store = Store(data)
        entries = {e['key']: e for e in manifest['entries']}
        for row in rows:
            if row['action'] != 'import':
                continue
            entry = entries[row['key']]
            # Recheck at the write boundary: a reviewer/importer may have added a family since planning.
            fresh = plan(dict(schema=manifest['schema'], entries=[entry]), workspace, data)[0]
            if fresh['action'] != 'import':
                row.update(fresh)
                continue
            source = Path(fresh['source'])
            item = store.add(source.read_bytes(), entry['title'], entry['version'], entry['provenance'],
                             status=entry.get('status', 'pending'), revision_info=dict(
                                 catalog_key=entry['key'], historical=True, source_path=str(source),
                                 source_sha256=entry['source_sha256'], evidence_files=entry['evidence_files'],
                                 approval_record=entry.get('approval_record'), limitations=entry.get('limitations', ''),
                                 evidence_views=entry.get('evidence_views', [])))
            row.update(action='imported', id=item['id'], status=item['status'])
        store.write_queue()
    return dict(schema='rekhcom.art-history-report/1', at=now(), applied=apply, entries=rows,
                backlog=manifest.get('backlog', []), counts={a: sum(r['action'] == a for r in rows)
                    for a in sorted({r['action'] for r in rows})})


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--manifest', type=Path, default=ROOT / 'port/progress/ARTE_HISTORICO.json')
    p.add_argument('--workspace', type=Path, default=ROOT.parent, help='Parent containing the source worktrees')
    p.add_argument('--data', type=Path, default=ROOT / 'build/art-review')
    p.add_argument('--apply', action='store_true', help='Import validated missing families; default only plans')
    p.add_argument('--report', type=Path, help='Optional JSON report; does not submit human feedback')
    a = p.parse_args()
    report = run(json.loads(a.manifest.read_text(encoding='utf-8')), a.workspace, a.data, a.apply)
    if a.report:
        a.report.parent.mkdir(parents=True, exist_ok=True)
        a.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
