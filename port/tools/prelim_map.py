"""An event background's layers, each drawn alone with its tile grid (for writing Preliminar recipes):
build/prelim/map-<scene>-<bg>.png, 4x, a label every 4 tiles.

    python port/tools/prelim_map.py <scene> [--rows 10:32] [--scale 4]
"""
import argparse
import re
import sys
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'port/tools'))
import scene_tile_data as s  # noqa: E402


def main():
    p = argparse.ArgumentParser()
    p.add_argument('scene', type=int)
    p.add_argument('--rows', default='0:64')
    p.add_argument('--scale', type=int, default=4)
    a = p.parse_args()
    s.ROM = (ROOT / 'roms/B8CP.gba').read_bytes()
    syms = (ROOT / 'build/eu/pc/rom_symbols.s').read_text()
    name = 'gEventBg%03d' % a.scene
    b = s.load_bg(int(re.search(r'\.set _' + name + r', (0x[0-9a-fA-F]+)', syms)[1], 16))
    r0, r1 = (int(v) for v in a.rows.split(':'))
    out = ROOT / 'build/prelim'
    out.mkdir(parents=True, exist_ok=True)
    for layer in ('bg3', 'bg2', 'bg1'):
        g = b[layer]
        if not g:
            continue
        w = len(g[0])
        im = Image.new('RGB', (w * 8, (r1 - r0) * 8), (40, 40, 48))
        td = b['tiles2'] if layer == 'bg1' and b['tiles2'] else b['tiles']
        for y in range(r0, min(r1, len(g))):
            for x, e in enumerate(g[y]):
                for yy in range(8):
                    for xx in range(8):
                        fx = 7 - xx if e & 1024 else xx
                        fy = 7 - yy if e & 2048 else yy
                        o = (e & 1023) * 32 + fy * 4 + fx // 2
                        ci = (td[o] >> (fx % 2 * 4)) & 15 if o < len(td) else 0
                        if ci:
                            im.putpixel((x * 8 + xx, (y - r0) * 8 + yy), b['colors'][(e >> 12) * 16 + ci])
        k = a.scale
        im = im.resize((im.width * k, im.height * k), Image.NEAREST)
        d = ImageDraw.Draw(im)
        for t in range(w + 1):
            d.line((t * 8 * k, 0, t * 8 * k, im.height), fill=(255, 255, 0) if t % 4 == 0 else (90, 90, 0))
            if t % 4 == 0:
                d.text((t * 8 * k + 2, 2), str(t), fill='white')
        for r in range(r0, r1 + 1):
            y = (r - r0) * 8 * k
            d.line((0, y, im.width, y), fill=(255, 255, 0) if r % 4 == 0 else (90, 90, 0))
            d.text((2, y + 2), str(r), fill='white')
        path = out / ('map-%03d-%s.png' % (a.scene, layer))
        im.save(path)
        print(path, im.size)


if __name__ == '__main__':
    main()
