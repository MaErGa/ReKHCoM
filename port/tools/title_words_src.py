#!/usr/bin/env python3
"""The title's word sprites as the cartridge draws them (menu entries, selected entries, Brillo banners, credit,
cursor), every frame and language, decoded from the ROM (config/assets/sprites_title.yaml) into build/title-words/<version>/
<frame symbol>.png with their palette, in frame coordinates (box in index.tsv). The reference make_title_words.py
traces to draw the HD words; ROM-derived pictures stay under build/ and are never committed.

    python port/tools/title_words_src.py [eu us jp]
"""
import sys
from pathlib import Path

import yaml
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import sprite_sheet  # noqa: E402

ROMS = {'eu': 'B8CP', 'us': 'B8CE', 'jp': 'B8CJ'}
SHEETS = ('gTitleMenuEntry', 'gTitleMenuSelectedEntry', 'gTitleSoraLumi', 'gTitleRikuLumi', 'gTitleDevelopedBy',
          'gTitleMenuCursor')
# the palette title.c loads for each sheet (palette, 16-colour bank)
PALETTE = {'gTitleMenuEntry': ('gTitleMenuEntryPalette', 0), 'gTitleMenuSelectedEntry': ('gTitleObjPalettes', 0),
           'gTitleMenuCursor': ('gTitleObjPalettes', 0), 'gTitleSoraLumi': ('gTitleSoraLumiPalette', 0),
           'gTitleRikuLumi': ('gTitleSoraLumiPalette', 1),  # Riku's title: palette offset 0x20 (title.c)
           'gTitleDevelopedBy': ('gTitleLogoKingdomHeartsPalette', 0)}


def rgb(pal, i):
    c = pal[i * 2] | pal[i * 2 + 1] << 8
    return ((c & 31) * 255 // 31, (c >> 5 & 31) * 255 // 31, (c >> 10 & 31) * 255 // 31)


def main():
    versions = sys.argv[1:] or ['eu', 'us', 'jp']
    entries = {}
    for f in (ROOT / 'config' / 'assets').glob('sprites_title.yaml'):
        for e in yaml.safe_load(f.read_text(encoding='utf-8'))['entries']:
            entries[e['name']] = e
    for version in versions:
        rom = (ROOT / 'roms' / f'{ROMS[version]}.gba').read_bytes()
        out = ROOT / 'build' / 'title-words' / version
        out.mkdir(parents=True, exist_ok=True)
        rows = []
        for name, e in entries.items():
            if e.get('format') != 'sprite_sheet' or version not in e or not name.startswith(SHEETS):
                continue
            base = next(s for s in SHEETS if name.startswith(s))
            pname, bank = PALETTE[base]
            pe = entries[pname][version]
            pa = pe['address'] - 0x08000000 + bank * 32  # DrawSprite's palette replaces the pieces' own banks
            pal = rom[pa:pa + 32]
            a = e[version]['address'] - 0x08000000
            data = rom[a:a + e[version]['size']]
            frames, _anims, block = sprite_sheet.parse_records(data, len(e['frames']), len(e.get('animations', [])))
            for fi, (desc, oam) in enumerate(zip(e['frames'], frames)):
                pieces = [sprite_sheet.piece_from_oam(o) for o in oam]
                if not pieces:
                    continue
                x0 = min(p['x'] for p in pieces); y0 = min(p['y'] for p in pieces)
                x1 = max(p['x'] + p['w'] for p in pieces); y1 = max(p['y'] + p['h'] for p in pieces)
                im = Image.new('RGBA', (x1 - x0, y1 - y0), (0, 0, 0, 0))
                idx = Image.new('L', im.size, 0)  # palette index (+16 per bank), for tracing the parts
                for p in reversed(pieces):  # OAM order: the first piece is on top
                    t = sprite_sheet.flipped(sprite_sheet.tile_rows(block, p['tile'], p['w'], p['h']), p['hflip'], p['vflip'])
                    for yy, row in enumerate(t):
                        for xx, v in enumerate(row):
                            if v:
                                im.putpixel((p['x'] - x0 + xx, p['y'] - y0 + yy), rgb(pal, v) + (255,))
                                idx.putpixel((p['x'] - x0 + xx, p['y'] - y0 + yy), p['pal'] * 16 + v)
                sym = desc['symbol']
                im.save(out / f'{sym}.png')
                idx.save(out / f'{sym}.idx.png')
                rows.append(f'{name}\t{sym}\t{x0}\t{y0}\t{x1 - x0}\t{y1 - y0}')
        (out / 'index.tsv').write_text('\n'.join(rows) + '\n', encoding='utf-8')
        print(version, len(rows), 'frames ->', out)


if __name__ == '__main__':
    main()
