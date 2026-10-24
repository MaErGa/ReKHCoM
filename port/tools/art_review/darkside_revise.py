"""Darkside's poses, next revision from the human clouds (24/10): one child version per reviewed image, imported into
the review desk with the cloud it answers. Never touches the game, the ROM or the source .inc (a proposal .inc is
written next to the study for a later integration, only once approved with honours).

Learned rules applied to every pose (from the clouds): additions only from x256 rightwards (03); no mirrored halves
(04); the added part must follow the sprite's own logic, never a second copy of a limb already drawn (05); fitted to the
original at the seam (03 v2, 06).

    python port/tools/art_review/darkside_revise.py --source D:/01Claude/arenas-wt --data <review data> [--only 03,06]
"""
import argparse
import hashlib
import json
import re
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from revise_darkside_boundary import frame, extension
from server import Store, now

SEAM = 256
BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]
NATURAL_BG = '#e8e9df'


def latest_submission(data, image_id):
    """the newest submission with clouds on this image"""
    best = None
    for path in sorted((data / 'envios').glob('*/comentarios.json')):
        payload = json.loads(path.read_text(encoding='utf-8'))
        for a in payload['assets']:
            if a['id'] == image_id and a['annotations']:
                best = (payload, a)
    if best is None:
        raise ValueError('No submission with clouds for ' + image_id)
    return best


def seam_score(original, added):
    """continuity at x255|x256: rows where both sides are drawn, minus rows where only one is (a step in the outline)"""
    ys = {y for x, y in original if x >= SEAM - 2} | {y for x, y in added if x <= SEAM + 1}
    good = bad = 0
    for y in ys:
        left = any((x, y) in original for x in (SEAM - 2, SEAM - 1))
        right = any((x, y) in added for x in (SEAM, SEAM + 1))
        good += left and right
        bad += left != right
    return good - 2 * bad


def shifted(added, dx, dy):
    return {(x + dx, y + dy): c for (x, y), c in added.items()}


def best_fit(original, added, dxs, dys):
    """the shift of the added drawing that best continues the original's outline at the seam"""
    best = None
    for dy in dys:
        for dx in dxs:
            cand = {p: c for p, c in shifted(added, dx, dy).items() if p[0] >= SEAM and p not in original}
            score = seam_score(original, cand)
            if best is None or score > best[0] or (score == best[0] and abs(dx) + abs(dy) < abs(best[1]) + abs(best[2])):
                best = (score, dx, dy, cand)
    return best


def outline(pixels, dark):
    """the game's own dark rim on the drawing's open edges (right, top and bottom of what is kept)"""
    out = dict(pixels)
    for (x, y) in pixels:
        if x < SEAM:
            continue
        if any((x + dx, y + dy) not in pixels for dx, dy in ((1, 0), (0, -1), (0, 1))):
            out[x, y] = dark
    return out


def faded(added, width):
    """the added part ends in shadow: ordered dither, thinning toward its far edge (the game has no alpha in this layer)"""
    out = {}
    for (x, y), c in added.items():
        level = max(0.0, 1.0 - (x - SEAM) / max(1, width))
        if level * 16 > BAYER[y % 4][x % 4]:
            out[x, y] = c
    return out


def edge_rows(original):
    """the original's rows that reach the block's cut at x255, with the colour there"""
    return {y: original[x, y] for (x, y) in original if x == SEAM - 1}


def rounded_back(original, added, depth):
    """the back closed by a rounded contour: on the rows cut at x255, as far as an ellipse over those rows allows"""
    rows = sorted(edge_rows(original))
    if not rows:
        return {}
    top, bottom = rows[0], rows[-1]
    cy, ry = (top + bottom) / 2, max(1, (bottom - top) / 2)
    out = {}
    for (x, y), c in added.items():
        if x < SEAM or not top <= y <= bottom:
            continue
        reach = depth * max(0.0, 1 - ((y - cy) / ry) ** 2) ** 0.5
        if x - SEAM < reach:
            out[x, y] = c
    return out


def shadow_tail(original, palette, length):
    """the arm comes out of the dark: each row cut at x255 goes on in its own colour, then the darkest, thinning out"""
    dark = darkest(palette, set(original.values()))
    out = {}
    for y, c in edge_rows(original).items():
        if not all((SEAM - 1 - k, y + d) in original for k in range(6) for d in (-1, 0, 1)):
            continue  # a loose strand of hair, not the arm's mass
        for k in range(length):
            level = 1.0 - k / length
            if level * 16 > BAYER[y % 4][(SEAM + k) % 4]:
                out[SEAM + k, y] = c if k < length // 4 else dark
    return out


def darkest(palette, used):
    return min(used, key=lambda i: sum(palette[i]))


def pack_inc(old_inc, name, pixels, top_define):
    """the proposal as the game's .inc: rows from DARKSIDE_<NAME>_TOP, 64 bytes (128 px from x232), two pixels a byte"""
    top = int(re.search(r'#define %s (\d+)' % top_define, old_inc)[1])
    rows = len(re.findall(r'\{\s*(0x[^}]+)\}', old_inc))
    lines = []
    for r in range(rows):
        b = []
        for j in range(64):
            lo = pixels.get((232 + j * 2, top + r), 0)
            hi = pixels.get((232 + j * 2 + 1, top + r), 0)
            b.append('0x%02x' % (lo | hi << 4))
        lines.append('    { ' + ', '.join(b) + ' },')
    body = re.sub(r'(=\s*\{\n)(.*?)(\n\};)', lambda m: m[1] + '\n'.join(lines) + m[3], old_inc, count=1, flags=re.S)
    return body


def draw(path, title, lines, original, added, palette, extra=None):
    natural = Image.new('RGB', (384, 256), NATURAL_BG)
    prov = natural.copy()
    for (x, y), c in {**added, **original}.items():
        if 0 <= x < 384 and 0 <= y < 256:
            natural.putpixel((x, y), palette[c])
            prov.putpixel((x, y), (54, 126, 175) if (x, y) in original else (232, 135, 60))
    pts = list(original) + list(added)
    box = (max(0, min(x for x, y in pts) - 8), max(0, min(y for x, y in pts) - 8),
           min(384, max(x for x, y in pts) + 9), min(256, max(y for x, y in pts) + 9))
    w, h = (box[2] - box[0]) * 2, (box[3] - box[1]) * 2
    natural = natural.crop(box).resize((w, h), Image.NEAREST)
    prov = prov.crop(box).resize((w, h), Image.NEAREST)
    panes = [natural, prov] + ([extra] if extra is not None else [])
    pane = max(420, w)
    fig = Image.new('RGB', (pane * len(panes) + 24 * (len(panes) + 1), h + 180), '#fafbf5')
    d = ImageDraw.Draw(fig)
    font = ImageFont.truetype('C:/Windows/Fonts/arial.ttf', 22)
    small = ImageFont.truetype('C:/Windows/Fonts/arial.ttf', 16)
    d.text((24, 17), title, font=font, fill='#274d43')
    d.text((24, 53), 'Colores de la ROM', font=small, fill='#526653')
    d.text((pane + 48, 53), 'AZUL = original   NARANJA = añadido', font=small, fill='#526653')
    for i, im in enumerate(panes):
        fig.paste(im.resize((im.width * h // im.height, h), Image.NEAREST) if im.height != h else im, (24 + i * (pane + 24), 84))
    seam = pane + 48 + (SEAM - box[0]) * 2
    for y in range(84, 84 + h, 12):
        d.line((seam, y, seam, min(y + 6, 84 + h)), fill='#bd512a', width=2)
    for k, line in enumerate(lines):
        d.text((24, h + 100 + 22 * k), line, font=small, fill='#526653')
    fig.save(path)


POSES = {  # image title prefix -> (.inc name, define, ROM frame slot)
    '03': ('raise', 'DARKSIDE_RAISE_TOP', 41),
    '05': ('lean', 'DARKSIDE_LEAN_TOP', 8),
    '06': ('punch', 'DARKSIDE_PUNCH_TOP', 13),
    '07': ('reach', 'DARKSIDE_REACH_TOP', 10),
}


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--source', type=Path, required=True)
    p.add_argument('--data', type=Path, required=True)
    p.add_argument('--only', default='03,05,06,07')
    args = p.parse_args()
    store = Store(args.data)
    assets = store.all()
    out = args.data / 'proposals' / 'darkside-r3'
    out.mkdir(parents=True, exist_ok=True)
    made = []
    for key in args.only.split(','):
        name, define, slot = POSES[key]
        family = [a for a in assets if a['title'].startswith('Darkside ' + key)]
        reviewed = [a for a in family if a['annotations']][-1]  # the newest version with clouds
        payload, sent = latest_submission(args.data, reviewed['id'])
        inc_path = args.source / 'port/include/port' / ('darkside_body_%s.inc' % name)
        inc = inc_path.read_text()
        original, palette = frame(args.source, slot)
        added = {pt: c for pt, c in extension(inc).items() if pt not in original}
        if key == '03':  # v2 already kept x>=256 only; the cloud: a little higher/lower so the drawing fits
            base = {pt: c for pt, c in added.items() if pt[0] >= SEAM}
            # chosen by eye among -16..0 (the seam score could not tell them apart): the shoulder meets the chest
            dy = -6
            new = {pt: c for pt, c in shifted(base, 0, dy).items() if pt[0] >= SEAM and pt not in original}
            change = 'El brazo añadido subido 6 px: el hombro empalma con el pectoral (antes quedaba un hueco debajo).'
        elif key == '06':  # the arm is right but badly fitted: the shift that continues the shoulder
            base = {pt: c for pt, c in added.items() if pt[0] >= SEAM - 16}
            dx, dy = -6, 6  # chosen by eye: the arm leaves the shoulder's curve instead of hanging from the hair
            new = {pt: c for pt, c in shifted(base, dx, dy).items() if pt[0] >= SEAM and pt not in original}
            change = 'El brazo añadido 6 px más abajo y 6 px hacia el cuerpo: sale de la curva del hombro, no del pelo.'
        elif key == '05':  # the arm with claws is already on the left: the right side is only the back, closed
            keep = rounded_back(original, added, 24)
            new = outline(keep, darkest(palette, set(original.values())))
            change = 'Quitado el brazo duplicado (ya está a la izquierda con las garras); a la derecha solo la espalda, redondeada y cerrada con el contorno oscuro del juego.'
        else:  # 07: the reaching arm ends in shadow, an attack of the dark
            new = shadow_tail(original, palette, 28)
            change = 'Sin cuerpo añadido: es el brazo atacando; donde el original se corta, sigue unos píxeles y se disuelve en sombra (tramado).'
        new = {pt: c for pt, c in new.items() if pt not in original and pt[0] >= SEAM}
        assert all(pt[0] >= SEAM for pt in new)
        study = out / ('darkside-%s-r3.png' % key)
        extra = None
        title = '%s  DARKSIDE · %s · revisión siguiente' % (key, reviewed['title'].split('·')[1].strip())
        lines = ['Respuesta a: «%s»' % sent['annotations'][-1]['comment'].strip()[:110], 'Cambio: ' + change,
                 'Original intacto; añadidos solo desde x256. Pendiente de tu revisión; no integrado en el juego.']
        if key == '07':  # the original alone, as asked
            alone = Image.new('RGB', (384, 256), NATURAL_BG)
            for (x, y), c in original.items():
                alone.putpixel((x, y), palette[c])
            pts = list(original)
            box = (min(x for x, y in pts) - 8, min(y for x, y in pts) - 8, max(x for x, y in pts) + 9, max(y for x, y in pts) + 9)
            extra = alone.crop(box).resize(((box[2] - box[0]) * 2, (box[3] - box[1]) * 2), Image.NEAREST)
            lines[2] = 'Tercera imagen: el pixel art original solo (fotograma EU 10). ' + lines[2]
        draw(study, title, lines, original, new, palette, extra)
        proposal = pack_inc(inc, name, new, define)
        (out / ('darkside_body_%s_r3.inc' % name)).write_text(proposal, encoding='utf-8')
        info = dict(created=now(), response_to=[dict(submission=payload['submission_id'], image_id=sent['id'],
                    cloud_id=n['id'], reviewed_image_sha256=sent['sha256']) for n in sent['annotations']],
                    change=change, technique='Índices 4bpp nativos sobre el fotograma de la ROM; reglas aprendidas de las nubes.',
                    source_inc_sha256=hashlib.sha256(inc.encode()).hexdigest(),
                    proposal_inc=str(out / ('darkside_body_%s_r3.inc' % name)), subsequent_human_verdict=None)
        item = store.add(study.read_bytes(), reviewed['title'].split(' · v')[0] + ' · r3',
                         'Revisión 3 · ' + change[:120], 'Respuesta a ' + ', '.join(n['id'] for n in sent['annotations']) +
                         '. Azul original, naranja añadido. Propuesta privada; no integrada.',
                         parent_id=reviewed['id'], revision_info=info)
        made.append(dict(pose=key, id=item['id'], study=str(study), change=change))
    print(json.dumps(made, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
