#!/usr/bin/env python3
"""Re:CoM's words and pictures in their own proportion (user 26/10: the battle notices and the menus' banners were
squeezed into the cartridge's box): every recipe of port/data/hd_sprites.tsv whose Re:CoM part is drawn narrower than
it is keeps its height and gets its width back, the box going on past the cartridge's on the side that held it
(pad=L,R: port_hdsprites.c draws the overhang from the edge parts):

- a battle notice / level-up word (group 3, one blit): wider on both sides, centred where it was;
- a menu banner (group 5): its word (or its letters, each in its own proportion, the gaps kept) runs on to the right
  and the banner's bar and arrow go with it;
- a part drawn wider than it is (a face, a logo): narrowed, centred.

The cards' value digits (group 1) are left to the card digits' own recipe. Without --write: the changes listed.

    python port/tools/fix_hd_aspect.py [--write]
"""
import math
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TSV = ROOT / 'port/data/hd_sprites.tsv'
K = 8          # recipe pixels per GBA pixel
TOL = 0.08     # proportions within 8% are left alone


def nums(v):
    return [int(round(float(x))) for x in re.findall(r'-?\d+(?:\.\d+)?', v)]


def parse(rec):
    return [op.split('=', 1) if '=' in op else [op, None] for op in rec.split(';')]


def join(ops):
    return ';'.join(k if v is None else f'{k}={v}' for k, v in ops)


def blit_ratio(v):
    sw, sh, dw, dh = v[2] - v[0], v[3] - v[1], v[6] - v[4], v[7] - v[5]
    return sw / sh, dw / dh


def fix_notice(ops, box_w):
    """group 3, one blit: the word as wide as its height asks, centred; the canvas grows on both sides"""
    i = next(n for n, (k, v) in enumerate(ops) if k == 'blit')
    v = nums(ops[i][1])
    src, dst = blit_ratio(v)
    if dst >= src * (1 - TOL):
        return None
    cw, ch = nums(next(v2 for k, v2 in ops if k == 'canvas'))
    dh = v[7] - v[5]
    dw = round(dh * src)
    cx = (v[4] + v[6]) / 2
    x0, x1 = round(cx - dw / 2), round(cx - dw / 2) + dw
    over = max(0, -x0, x1 - cw)
    pad = math.ceil(over / K) + 1 if over else 0
    if not pad:
        return None
    v[4], v[6] = x0 + pad * K, x1 + pad * K
    ops[i][1] = ','.join(map(str, v))
    for n, (k, val) in enumerate(ops):
        if k == 'canvas':
            ops[n][1] = f'{cw + 2 * pad * K},{ch}'
    ops.insert(0, ['pad', f'{pad},{pad}'])
    return f'notice {src:.2f} (was {dst:.2f}), pad {pad}+{pad}'


def fix_banner(ops):
    """group 5: the word (or its letters) in their proportion from where it starts, the bar and arrow pushed right"""
    texts = [n for n, (k, v) in enumerate(ops) if k in ('blit', 'sblit')]
    if not texts or not any(k == 'canvas' for k, v in ops):
        return None
    vs = [nums(ops[n][1]) for n in texts]
    if not any(blit_ratio(v)[1] < blit_ratio(v)[0] * (1 - TOL) for v in vs):
        return None
    # lay the parts out again left to right: each its own width at its height, the gaps (or overlaps) between kept
    order = sorted(range(len(vs)), key=lambda j: vs[j][4])
    shift, prev_old_end, prev_new_end = 0, None, None
    for j in order:
        v = vs[j]
        src, _ = blit_ratio(v)
        dh = v[7] - v[5]
        dw = max(v[6] - v[4], round(dh * src))
        gap = v[4] - prev_old_end if prev_old_end is not None else 0
        start = v[4] if prev_new_end is None else prev_new_end + gap
        prev_old_end = v[6]
        v[4], v[6] = start, start + dw
        prev_new_end = v[6]
        shift = max(shift, v[6] - prev_old_end)
    grow = shift
    if grow <= 0:
        return None
    pad = math.ceil(grow / K)
    for n, v in zip(texts, vs):
        ops[n][1] = ','.join(map(str, v))
    for n, (k, val) in enumerate(ops):
        if k == 'canvas':
            cw, ch = nums(val)
            ops[n][1] = f'{cw + pad * K},{ch}'
        elif k == 'poly':
            colour, *pts = val.split(',')
            pts = [int(p) for p in pts]
            pts = [p + grow if i % 2 == 0 and p > 0 else p for i, p in enumerate(pts)]
            ops[n][1] = ','.join([colour] + [str(p) for p in pts])
    ops.insert(0, ['pad', f'0,{pad}'])
    return f'banner +{grow} px ({pad} GBA px right)'


def fix_wide(ops):
    """a part drawn wider than it is: narrowed, centred"""
    note = []
    crop = None
    for n, (k, v) in enumerate(ops):
        if k == 'crop':
            c = nums(v)
            crop = (c[2] - c[0]) / (c[3] - c[1])
        elif k == 'place' and crop:
            p = nums(v)
            pw, ph = p[2] - p[0], p[3] - p[1]
            if pw / ph > crop * (1 + TOL):
                w = round(ph * crop)
                x0 = p[0] + (pw - w) // 2
                ops[n][1] = f'{x0},{p[1]},{x0 + w},{p[3]}'
                note.append(f'place {pw / ph:.2f}->{crop:.2f}')
        elif k in ('blit', 'sblit'):
            b = nums(v)
            src, dst = blit_ratio(b)
            if dst > src * (1 + TOL):
                w = round((b[7] - b[5]) * src)
                x0 = b[4] + (b[6] - b[4] - w) // 2
                b[4], b[6] = x0, x0 + w
                ops[n][1] = ','.join(map(str, b))
                note.append(f'{k} {dst:.2f}->{src:.2f}')
    return ', '.join(note) or None


def main():
    lines = TSV.read_text(encoding='utf-8').split('\n')
    changed = 0
    for i, line in enumerate(lines):
        if not line.strip() or line.startswith('#'):
            continue
        row = line.split('\t')
        if len(row) < 10 or row[9] in ('raw', '') or 'pad=' in row[9]:
            continue
        group, ops = row[1], parse(row[9])
        if group == '1':
            continue
        notes = []
        if group == '3' and sum(k == 'blit' for k, v in ops) == 1 and any(k == 'canvas' for k, v in ops):
            notes.append(fix_notice(ops, int(row[6])))
        elif group == '5':
            notes.append(fix_banner(ops))
        notes.append(fix_wide(ops))
        notes = [n for n in notes if n]
        if notes:
            row[9] = join(ops)
            lines[i] = '\t'.join(row)
            changed += 1
            print(row[0], '|', '; '.join(notes))
    print(changed, 'rows')
    if '--write' in sys.argv:
        TSV.write_text('\n'.join(lines), encoding='utf-8')


if __name__ == '__main__':
    main()
