"""A Preliminar scene's next version for the review desk: the shot build/prelim_shot.sh made (build/prelim/<scene>-after.png)
imported as the child of the reviewed image, with the clouds of its newest submission marked as answered.

    python port/tools/art_review/answer_scene.py <scene> --change "what changed" [--data <review data>]
"""
import argparse
import json
import tempfile
from pathlib import Path
from server import Store, now

ROOT = Path(__file__).resolve().parents[3]


def main():
    p = argparse.ArgumentParser()
    p.add_argument('scene', type=int)
    p.add_argument('--change', required=True)
    p.add_argument('--data', type=Path, default=ROOT / 'build/art-review')
    args = p.parse_args()
    store = Store(args.data)
    title = 'Escena %03d' % args.scene
    family = [a for a in store.all() if a['title'].startswith(title)]
    reviewed = [a for a in family if a['annotations']][-1]
    best = None
    for path in sorted((args.data / 'envios').glob('*/comentarios.json')):
        payload = json.loads(path.read_text(encoding='utf-8'))
        for a in payload['assets']:
            if a['id'] == reviewed['id'] and a['annotations']:
                best = (payload, a)
    payload, sent = best
    shot = ROOT / 'build/prelim' / ('%d-after.png' % args.scene)
    info = dict(created=now(), response_to=[dict(submission=payload['submission_id'], image_id=sent['id'], cloud_id=n['id'],
                                                 reviewed_image_sha256=sent['sha256']) for n in sent['annotations']],
                change=args.change, technique='Receta Preliminar V1 (port_preliminar_v1_tiles.h), tiles de la ROM; '
                'captura 3440x1440 x3, cuadro 300 del evento (build/prelim_shot.sh).', subsequent_human_verdict=None)
    item = store.add(shot.read_bytes(), title, 'Revisión · ' + args.change[:120],
                     'Respuesta a ' + ', '.join(n['id'] for n in sent['annotations']) + '. Preliminar: no integrada.',
                     parent_id=reviewed['id'], revision_info=info)
    print(item['id'])


if __name__ == '__main__':
    main()
