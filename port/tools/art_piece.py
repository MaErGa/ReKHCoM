"""One art piece's brief, short enough to start working at once: the newest submission that reviewed it, every cloud
(text, expected result, box in the image's pixels and as a share of it, its crop file), the general comment and
strokes, the reviewed image, the code that draws that scene, and the commands to capture and answer.

    python port/tools/art_piece.py escena-090          # or: darkside-06, malefica-arena...
    python port/tools/art_piece.py escena-090 --json

Reads the shared review desk (--data, default the folder "Revisar preliminares.bat" uses). Never writes.
"""
import os
import argparse
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
DESK = Path(os.environ.get('ART_DESK') or REPO.parent / 'khcom-codex' / 'build' / 'art-review')

# What draws each kind of scene (found by the first worker round; LOGICA_APROBADOS has the rules)
MECHANISMS = [
    (r'^escena-0*4$', 'port/src/port_ppu.c (IsoEntry/IsoPrepare: the generic isometric fill, shared with other scenes;'
                      ' give the scene a profile of its own instead of changing the shared one)'),
    (r'^escena-(057|061|074)$', 'port/include/port/port_scene{n}_tiles.h + port/include/port/port_scene_wide_tiles.h'),
    (r'^escena-\d+$', 'port/include/port/port_preliminar_v1_tiles.h (PortPreliminarTileSource/PixelSource) and the '
                      'table port/include/port/port_preliminar_v1_data.inc (generated: '
                      'port/tools/preliminar_v1_recipes.py --generate <inventory>; ready=0 rows are not applied); '
                      'map dump: port/tools/prelim_map.py'),
    (r'^darkside', 'port/tools/art_review/darkside_revise.py, revise_darkside_boundary.py; arena: '
                   'port/tools/arena_before_shots.py --v2 --only darkside (KHCOM_DARKSIDE_BEACH=1)'),
    (r'-arena$', 'port/tools/arena_before_shots.py --v2 --only <boss> (16:9 and 32:9); BATTLE_SCENERY_TECHNIQUE.md'),
    (r'^malefica|^oogie|^trickmaster', 'sprite study as Darkside: the sprite alone in the app (feedback: sprites aislados)'),
]


def title_for(piece):
    m = re.match(r'escena-0*(\d+)$', piece)
    if m:
        return 'Escena %03d' % int(m.group(1))
    words = piece.replace('-', ' ').split()
    names = {'darkside': 'Darkside', 'malefica': 'Maléfica', 'oogie': 'Oogie Boogie', 'trickmaster': 'Trickmaster'}
    head = names.get(words[0], words[0].capitalize())
    rest = ' '.join(words[1:])
    return head + (' ' + rest if rest else '')


def newest_review(data, title):
    """(submission, asset) of the newest submission whose annotated or commented asset has this title."""
    best = None
    for path in sorted((data / 'envios').glob('*/comentarios.json')):
        payload = json.loads(path.read_text(encoding='utf-8'))
        for a in payload['assets']:
            name = a['title'].lower()
            if not (name.startswith(title.lower()) or all(w in name for w in title.lower().split())):
                continue
            if a.get('annotations') or (a.get('general') or '').strip() or a.get('doodles'):
                best = (payload, a, path.parent)
    return best


def brief(piece, data):
    title = title_for(piece)
    found = newest_review(data, title)
    out = {'piece': piece, 'title': title}
    if not found:
        out['error'] = 'no submission with comments for "%s" in %s' % (title, data / 'envios')
        return out
    payload, a, folder = found
    w, h = a['width'], a['height']
    out.update(submission=payload['submission_id'], image_id=a['id'], image_title=a['title'],
               image=str(folder / (a['id'] + '.png')), zones=str(folder / (a['id'] + '-zonas.png')),
               size=[w, h], general=(a.get('general') or '').strip(), strokes=len(a.get('doodles') or []),
               doodle_summary=a.get('doodle_summary'))
    out['clouds'] = [dict(id=c['id'], n=c.get('number'), comment=c.get('comment', ''), expected=c.get('expected') or '',
                          box_px=c.get('pixels_xyxy_exclusive'),
                          box_share=[round(c['x'], 3), round(c['y'], 3), round(c['w'], 3), round(c['h'], 3)],
                          crop=str(folder / c['crop']) if c.get('crop') else None)
                     for c in a.get('annotations') or []]
    out['code'] = [text.replace('{n}', piece.split('-')[-1].zfill(3))
                   for pattern, text in MECHANISMS if re.search(pattern, piece)][:2]
    n = re.match(r'escena-0*(\d+)$', piece)
    shot = ('sh build/prelim_shot.sh %d   # 3440x1440 zoom 6, frame 300, before/after' % int(n.group(1))) if n else \
        'see the code line above'
    out['capture'] = shot
    out['answer'] = ('python port/tools/art_review/import_image.py <png> --parent %s --data "%s" '
                     '--revision-info <json file: {"response_to":[{"submission":"%s","image_id":"%s","cloud_id":<id>}]}>'
                     % (a['id'], data, payload['submission_id'], a['id']))
    if n:
        out['answer_scene'] = ('python port/tools/art_review/answer_scene.py %d --change "..." --data "%s"   '
                               '(marks EVERY cloud answered: only when all are)' % (int(n.group(1)), data))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('piece')
    ap.add_argument('--data', type=Path, default=DESK)
    ap.add_argument('--json', action='store_true')
    a = ap.parse_args()
    try:
        sys.stdout.reconfigure(encoding='utf-8')
    except (AttributeError, ValueError):
        pass
    b = brief(a.piece, a.data)
    if a.json or 'error' in b:
        print(json.dumps(b, ensure_ascii=False, indent=1))
        return 1 if 'error' in b else 0
    print('%s  ·  %s  ·  envío %s' % (b['piece'], b['image_title'], b['submission']))
    print('imagen revisada: %s (%dx%d)\nzonas: %s' % (b['image'], b['size'][0], b['size'][1], b['zones']))
    for c in b['clouds']:
        print('\nnube %s [%s]: %s' % (c['n'], c['id'], c['comment']))
        if c['expected']:
            print('  esperado: ' + c['expected'])
        print('  caja px %s · fracción x,y,w,h %s · recorte %s' % (c['box_px'], c['box_share'], c['crop']))
    if b['general']:
        print('\ncomentario general: ' + b['general'])
    if b['strokes']:
        print('trazos: %d %s' % (b['strokes'], b['doodle_summary'] or ''))
    print('\ncódigo: ' + ('\n        '.join(b['code']) or '(buscar)'))
    print('captura: ' + b['capture'])
    print('responder: ' + b['answer'])
    if 'answer_scene' in b:
        print('           ' + b['answer_scene'])
    return 0


if __name__ == '__main__':
    sys.exit(main())
