"""Fit drawn completion to the three native seams, preserving all ROM pixels.

Consumes the existing 41-state audit; emits palette-index candidates and mounted
reviews. No human rating is inferred or changed. Runtime installation is explicit.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[3]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--audit', type=Path, default=ROOT / 'build/maleficent-native-audit/manifest.json')
    ap.add_argument('--out', type=Path, default=ROOT / 'port/art/proposals/maleficent-continuity')
    ap.add_argument('--install-candidate', action='store_true')
    args = ap.parse_args()
    out = args.out
    out.mkdir(parents=True, exist_ok=True)
    audit = json.loads(args.audit.read_text())
    rom = (ROOT / 'roms/B8CP.gba').read_bytes()
    syms = (ROOT / 'build/eu/pc/rom_symbols.s').read_text()
    at = int(re.search(r'\.set _gBosMdPalette, 0x([0-9a-fA-F]+)', syms)[1], 16) - 0x8000000
    palette = [tuple(((w >> s) & 31) * 255 // 31 for s in (0, 5, 10))
               for w in struct.unpack_from('<16H', rom, at)]
    colours = np.asarray(palette, dtype=np.int32)
    base = np.asarray(Image.open(audit['frames'][0]['body']).convert('RGBA'))
    ys, xs = np.nonzero(base[::3, ::3, 3])
    xs, ys = xs * 3, ys * 3
    keep = xs < 220
    xs, ys = xs[keep], ys[keep]
    target = base[ys, xs, :3].astype(np.int32)
    reference = Image.open(out / 'reference.png').convert('RGBA')
    best = None
    for scale in (.24, .245, .25, .255, .26, .265):
        scaled = reference.resize((round(reference.width * scale), round(reference.height * scale)), Image.Resampling.NEAREST)
        pixels = np.asarray(scaled)
        for dx in range(35, 66):
            for dy in range(0, 21):
                xx, yy = xs - dx, ys - dy
                valid = (xx >= 0) & (yy >= 0) & (xx < scaled.width) & (yy < scaled.height)
                xx, yy = np.clip(xx, 0, scaled.width - 1), np.clip(yy, 0, scaled.height - 1)
                valid &= pixels[yy, xx, 3] >= 128
                delta = pixels[yy, xx, :3].astype(np.int32) - target
                score = np.where(valid, np.minimum((delta * delta).sum(axis=1), 10000), 10000).sum()
                if best is None or score < best[0]:
                    best = (score, scaled, dx, dy, scale)
    _, scaled, dx, dy, scale = best
    rgb = np.asarray(scaled)
    delta = rgb[:, :, None, :3].astype(np.int32) - colours[None, None, :, :]
    indices = (delta * delta).sum(axis=3)[:, :, 1:].argmin(axis=2).astype('uint8') + 1
    indices[rgb[:, :, 3] < 128] = 0
    master = np.zeros((384, 512), dtype='uint8')
    master[dy:dy + indices.shape[0], dx:dx + indices.shape[1]] = indices
    seam_rows = np.flatnonzero(master[:, 256])
    st, sb = int(seam_rows[0]), int(seam_rows[-1])
    packed_arrays, reports = [], []
    for number, group in enumerate(audit['cut_groups']):
        frame = audit['frames'][group['representative']]
        body = Image.open(frame['body']).convert('RGBA')
        native = np.asarray(body)
        edge = native[:, 255]
        rows = np.flatnonzero(edge[:, 3])
        top, bottom = int(rows[0]), int(rows[-1])
        edge_indices = np.zeros(384, dtype='uint8')
        for y in rows:
            edge_indices[y] = ((colours - edge[y, :3]) ** 2).sum(axis=1).argmin()
        completion = np.zeros((384, 512), dtype='uint8')
        # A narrow solid thorn, never a third wing. Match the high native cut,
        # then descend quickly to the drawn low back. Warp vanishes by x=304.
        for x in range(256, 512):
            distance = x - 256
            raw_rows = np.flatnonzero(master[:, x])
            if not len(raw_rows):
                continue
            raw_top = int(raw_rows[0])
            if distance <= 24:
                target_top = float(np.interp(distance, (0, 4, 8, 14, 24), (top, top - 14, top - 7, top + 27, top + 46)))
            else:
                weight = min(1., (distance - 24) / 24)
                target_top = (top + 46) * (1 - weight) + raw_top * weight
            for y in range(384):
                sy = round(raw_top + (y - target_top) * (sb - raw_top) / max(1., bottom - target_top)) if distance < 48 else y
                if not 0 <= sy < 384:
                    continue
                value = int(master[sy, x])
                if not value:
                    continue
                # The high addition is solid torso material, without membrane
                # veins or pale wing panels. Only native purple ramp indices.
                if distance < 25 and y < top + 49:
                    value = 2 if y < target_top + 2 else 5 if distance < 7 else 4 if distance < 16 else 3
                if distance == 0:
                    value = int(edge_indices[y])
                elif distance < 8 and edge_indices[y]:
                    weight = distance / 8
                    mixed = colours[edge_indices[y]] * (1 - weight) + colours[value] * weight
                    value = int(((colours[1:] - mixed) ** 2).sum(axis=1).argmin()) + 1
                completion[y, x] = value
        completion[:, 256] = edge_indices
        assert np.array_equal(completion[:, 256], edge_indices)
        (out / (group['id'] + '.indices')).write_bytes(completion.tobytes())
        rgba = np.zeros((384, 512, 4), dtype='uint8')
        rgba[:, :, :3] = colours[completion]
        rgba[:, :, 3] = np.where(completion, 255, 0)
        extension = Image.fromarray(rgba)
        original = Image.open(frame['mounted']).convert('RGBA')
        after = extension.copy()
        after.alpha_composite(original)
        padded_body = Image.new('RGBA', original.size)
        padded_body.alpha_composite(body)
        body_after = extension.copy()
        body_after.alpha_composite(body)
        for slot in group['frames']:
            pose = audit['frames'][slot]
            before = Image.open(pose['mounted']).convert('RGBA')
            pose_body = np.asarray(Image.open(pose['body']).convert('RGBA'))
            assert np.array_equal(pose_body[:, 255], edge), ('unexpected different seam', slot)
            merged = extension.copy()
            merged.alpha_composite(before)
            b, a = np.asarray(before), np.asarray(merged)
            assert np.array_equal(b[b[:, :, 3] != 0], a[b[:, :, 3] != 0])
        body.save(out / (group['id'] + '-native-body.png'))
        original.save(out / (group['id'] + '-native-mounted.png'))
        after.save(out / (group['id'] + '-mounted.png'))
        board = Image.new('RGB', (2048, 1440), '#d8d8df')
        draw = ImageDraw.Draw(board)
        for row, (left, right, label) in enumerate(((padded_body, body_after, 'CUERPO'), (original, after, 'MONTADO'))):
            draw.text((12, row * 704 + 8), f'Malefica {group["id"]} - {label} - ANTES / PROPUESTA NATIVA', fill='#171725')
            for col, im in enumerate((left, right)):
                im = im.resize((1024, 768), Image.Resampling.NEAREST).crop((0, 0, 1024, 640))
                board.paste(im, (col * 1024, row * 704 + 40), im)
        draw.text((12, 1420), '16 colores nativos; corte continuo; originales intactos. Propuesta pendiente de revision.', fill='#171725')
        board.save(out / (group['id'] + '-review.png'))
        yy, xx = np.nonzero(completion)
        x0, y0, x1, y1 = int(xx.min()), int(yy.min()), int(xx.max()) + 1, int(yy.max()) + 1
        values = completion[y0:y1, x0:x1].flatten().tolist()
        if len(values) % 2:
            values.append(0)
        packed = bytes(values[i] | values[i + 1] << 4 for i in range(0, len(values), 2))
        packed_arrays.append('static const uint8_t kMdCompletion%d[] = {\n%s\n};\n' % (number, '\n'.join('    ' + ','.join('0x%02x' % v for v in packed[i:i+24]) + ',' for i in range(0, len(packed), 24))))
        reports.append(dict(cut=group['id'], representative=group['representative'], frames=group['frames'], sources=group['sources'], bounds=[x0, y0, x1-x0, y1-y0], native_pixels_changed=0, seam_mismatches=0))
    include = '/* Candidate: native palette, original pixels retained; Atlas review pending. */\n' + '\n'.join(packed_arrays)
    (out / 'candidate.inc').write_text(include)
    if args.install_candidate:
        (ROOT / 'port/include/port/maleficent_body_approved.inc').write_text(include)
        path = ROOT / 'port/src/port_maleficent.c'
        text = path.read_text()
        for n, report in enumerate(reports):
            x, y, w, h = report['bounds']
            text, count = re.subn(r'pixels=kMdCompletion%d;x0=\d+;y0=\d+;w=\d+;h=\d+;' % n, f'pixels=kMdCompletion{n};x0={x};y0={y};w={w};h={h};', text)
            assert count == 1
        path.write_text(text)
    manifest = dict(status='pending', reference_sha256=hashlib.sha256((out/'reference.png').read_bytes()).hexdigest(), registration=dict(scale=scale, offset=[dx,dy]), palette=palette, groups=reports, states_checked=sum(len(g['frames']) for g in reports), native_pixels_changed=0, seam_mismatches=0, limitations=['Offline native-index validation; runtime animation capture still separate.', 'No human approval inherited from generated studies.'])
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2))
    print(json.dumps({k:manifest[k] for k in ('registration','states_checked','native_pixels_changed','seam_mismatches')}))


if __name__ == '__main__':
    main()
