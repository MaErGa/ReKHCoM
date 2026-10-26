#!/usr/bin/env python3
"""The battle cards' values in HD (port/data/hd_sprites.tsv, group 1), as big on the card as Re:CoM draws them (user
26/10: its value fills the card's round corner; ours were small): Re:CoM's digit (UK_Cockpit.imz) scaled up around
where the cartridge's digit sits, its box going on past the cartridge's 8x8 (pad), so it moves, turns and fades with
the card's own sprite. Every kind the game draws gets its row, with the cartridge's colours:

- normal (gCardValueDigitTiles): Re:CoM's white digit;
- boosted (gCardModifiedValueDigitTiles): white with the cartridge's blue outline;
- premium (gCardPremiumValueDigitTiles): the cartridge's gold, yellow to orange, dark red outline.

The boosted and premium tiles are drawn with two frame sets (a card in play: their own frames; in the deck stack:
gCardValueDigitFrames), so each has a row for both.

    python port/tools/card_digit_recipes.py [--write]
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TSV = ROOT / 'port/data/hd_sprites.tsv'
SOURCE = 'FORM/0002/FO0002.RTM:UK_Cockpit.imz#0'
PAD = 2          # GBA pixels past the 8x8 box on each side
GROW = 1.45      # Re:CoM's value against the cartridge's (its height on the card)
K = 8
BOOSTED = 'duo=FFFFFF,FFFFFF,2878F8'
PREMIUM = 'duo=F8F040,F86000,700008'


# the 8x8 rows as they were fitted to the cartridge's digits (Re:CoM's glyph rectangle, then where it lay in the 64x64
# picture): normal and boosted; premium lies as normal. Kept here so the script can run again.
ORIGINAL = {
    ('Card', 0): (2,38,42,76,14,8,64,64),
    ('CardModified', 0): (2,38,42,76,6,0,64,64),
    ('Card', 1): (46,38,72,76,21,8,59,64),
    ('CardModified', 1): (46,38,72,76,14,0,58,64),
    ('Card', 2): (75,38,115,76,14,8,64,64),
    ('CardModified', 2): (75,38,115,76,6,0,64,64),
    ('Card', 3): (121,38,161,76,14,8,64,64),
    ('CardModified', 3): (121,38,161,76,6,0,64,64),
    ('Card', 4): (164,38,201,76,16,8,64,64),
    ('CardModified', 4): (164,38,201,76,9,0,64,64),
    ('Card', 5): (206,38,246,76,14,8,64,64),
    ('CardModified', 5): (206,38,246,76,6,0,64,64),
    ('Card', 6): (249,38,289,76,14,8,64,64),
    ('CardModified', 6): (249,38,289,76,6,0,64,64),
    ('Card', 7): (293,38,329,76,18,8,64,64),
    ('CardModified', 7): (293,38,329,76,10,0,64,64),
    ('Card', 8): (334,38,374,76,14,8,64,64),
    ('CardModified', 8): (334,38,374,76,6,0,64,64),
    ('Card', 9): (378,38,415,76,16,8,64,64),
    ('CardModified', 9): (378,38,415,76,9,0,64,64),
}


def main():
    lines = TSV.read_text(encoding='utf-8').split('\n')
    old = ORIGINAL
    rows = []
    for n in range(10):
        src = old[('Card', n)][:4]
        for kind, tiles, frames, ops in (
                ('Card', 'gCardValueDigitTiles', ['gCardValueDigitFrame'], ''),
                ('CardModified', 'gCardModifiedValueDigitTiles', ['gCardValueDigitFrame', 'gCardModifiedValueDigitFrame'],
                 BOOSTED),
                ('CardPremium', 'gCardPremiumValueDigitTiles', ['gCardValueDigitFrame', 'gCardPremiumValueDigitFrame'],
                 PREMIUM)):
            # where the cartridge's digit of this kind sits in its 8x8 (the old rows were fitted to it): its centre,
            # the premium digit's ink lying as the normal one's
            d = old[(kind if kind != 'CardPremium' else 'Card', n)]
            cx, cy = (d[4] + d[6]) / 2 + PAD * K, (d[5] + d[7]) / 2 + PAD * K
            n0 = old[('Card', n)]  # one size for every kind: the normal digit's, grown
            w, h = (n0[6] - n0[4]) * GROW, (n0[7] - n0[5]) * GROW
            x0, y0 = round(cx - w / 2), round(cy - h / 2)
            dst = f'{x0},{y0},{x0 + round(w)},{y0 + round(h)}'
            side = (8 + 2 * PAD) * K
            for i, frame in enumerate(frames):
                name = f'g{kind}ValueDigitTiles_{n}' + ('_play' if i else '')
                recipe = ';'.join(x for x in (f'pad={PAD},{PAD},{PAD},{PAD}', f'canvas={side},{side}',
                                                f'blit={",".join(map(str, src))},{dst}', ops, 'scale=8') if x)
                rows.append('\t'.join([name, '1', f'{frame}{n}', tiles, '3', '8', '8', '8', SOURCE, recipe]))
    # the digit rows replaced in place: the first old digit row's position takes the new block
    first = next(i for i, l in enumerate(lines) if re.match(r'g(Card|CardModified)ValueDigitTiles_\d\t', l))
    kept = [l for l in lines if not re.match(r'g(Card|CardModified|CardPremium)ValueDigitTiles_\d(_play)?\t', l)]
    kept[first:first] = rows
    print(len(rows), 'digit rows')
    if '--write' in sys.argv:
        TSV.write_text('\n'.join(kept), encoding='utf-8')
    else:
        print('\n'.join(rows[:6]))


if __name__ == '__main__':
    main()
