#!/usr/bin/env python3
"""The HD title's words (user 26/10: the whole title in HD with the HD HUD): every menu entry, selected entry, Brillo
banner, credit line and the cursor redrawn at 8x in the cartridge's place and colours. The words are set in KHMenu.otf
(the cartridge's lettering, embedded in the game) fitted word by word into the cartridge's ink; flat parts (the
square, the underlines, the banners) keep their shape, with their gradients smooth; the cursor is redrawn as a smooth
orb from its rings. Output: port/assets/hd/title/words/<frame>.png (embedded, core_assets.py) and
port/include/port/title_words_hd.inc / .h (their kHdSprites rows, group 7, and symbols).

    python port/tools/title_words_src.py eu us                 (the cartridge's frames, build/title-words)
    python port/tools/make_title_words.py
"""
import json
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / 'build' / 'title-words'
OUT = ROOT / 'port' / 'assets' / 'hd' / 'title' / 'words'
INC = ROOT / 'port' / 'include' / 'port' / 'title_words_hd.inc'
FONT = ROOT / 'port' / 'assets' / 'fonts' / 'KHMenu.otf'
S = 8

LANGS = {'': 'en', 'French': 'fr', 'Spanish': 'es', 'Italian': 'it', 'German': 'de'}
ENTRY = {
    'en': ['NEW GAME', 'LOAD', 'LINK', 'CONTINUE', 'NEW GAME : SORA', 'NEW GAME : RIKU'],
    'fr': ['NOUVELLE PARTIE', 'CHARGER', 'LINK', 'CONTINUER', 'NOUV.PARTIE : SORA', 'NOUV.PARTIE : RIKU'],
    'es': ['NUEVA PARTIDA', 'CARGAR', 'LINK', 'CONTINUAR', 'NUEVA PARTIDA : SORA', 'NUEVA PARTIDA : RIKU'],
    'it': ['NUOVO GIOCO', 'CARICA', 'LINK', 'CONTINUA', 'NUOVO GIOCO : SORA', 'NUOVO GIOCO : RIKU'],
    'de': ['NEUES SPIEL', 'LADEN', 'LINK', 'FORTSETZEN', 'NEUES SPIEL : SORA', 'NEUES SPIEL : RIKU'],
}
LUMI = {
    'en': ['DARKER', 'BRIGHTNESS', 'LIGHTER'], 'fr': ['SOMBRE', 'LUMINOSITÉ', 'CLAIR'],
    'es': ['OSCURO', 'BRILLO', 'CLARO'], 'it': ['MENO', 'LUMINOSITÀ', 'PIÙ'],
    'de': ['DUNKLER', 'HELLIGKEIT', 'HELLER'],
}
CREDIT = {
    'en': 'DEVELOPED BY SQUARE ENIX / Jupiter', 'fr': 'DÉVELOPPÉ PAR SQUARE ENIX / Jupiter',
    'es': 'DESARROLLADO POR SQUARE ENIX / Jupiter', 'it': 'SVILUPPATO DA SQUARE ENIX / Jupiter',
    'de': 'ENTWICKELT VON SQUARE ENIX / Jupiter',
}
PRESS = {'en': 'PRESS START', 'fr': 'APPUIE SUR START', 'es': 'PULSA START', 'it': 'PREMI START',
         'de': 'BITTE START DRÜCKEN'}
# regions each symbol exists in (the English names are shared by EU and US)
REGIONS = {'eu': 'VERSION_EU', 'us': 'VERSION_US'}


def lerp(a, b, t):
    return tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(len(a)))


def Sheet(frame):
    for k in ('gTitleMenuSelectedEntry', 'gTitleMenuEntry', 'gTitleDevelopedBy', 'gTitleMenuCursor'):
        if frame.startswith(k):
            return k + (frame[-6:] if k == 'gTitleMenuCursor' else '')
    return None


def lang_of(frame, kind):
    rest = frame[len(kind):frame.rindex('Frame')]
    return LANGS.get(rest)


def font_word(text, w, h):
    """text in KHMenu, its ink fitted to w x h: an alpha mask."""
    f = ImageFont.truetype(str(FONT), 256)
    if text in ('◀', '▶'):
        m = Image.new('L', (w, h), 0)
        d = ImageDraw.Draw(m)
        pts = [(w, 0), (0, h / 2), (w, h)] if text == '◀' else [(0, 0), (w, h / 2), (0, h)]
        d.polygon(pts, fill=255)
        return m
    big = Image.new('L', (256 * (len(text) + 2), 400), 0)
    ImageDraw.Draw(big).text((64, 40), text, font=f, fill=255)
    ink = big.crop(big.getbbox())
    natural = round(ink.width * h / ink.height)
    if w > natural * 1.3:  # a narrow letter (I) in a pixel cell as wide as a stroke pair: its own width, centred
        m = Image.new('L', (w, h), 0)
        m.paste(ink.resize((natural, h), Image.LANCZOS), ((w - natural) // 2, 0))
        return m
    return ink.resize((w, h), Image.LANCZOS)


def runs(cols, n):
    """the n runs of inked columns split at the n-1 widest gaps"""
    on = [x for x, v in enumerate(cols) if v]
    gaps = sorted(((on[i + 1] - on[i], on[i]) for i in range(len(on) - 1) if on[i + 1] - on[i] > 1), reverse=True)
    if len(gaps) < n - 1:
        return None
    cuts = sorted(g[1] for g in gaps[:n - 1])
    out, start = [], on[0]
    for c in cuts:
        out.append((start, c + 1))
        start = next(x for x in on if x > c)
    out.append((start, on[-1] + 1))
    return out


def gaps_runs(cols):
    out, x = [], 0
    while x < len(cols):
        if cols[x]:
            s = x
            while x < len(cols) and cols[x]:
                x += 1
            out.append((s, x))
        x += 1
    return out


def banner_tokens(word):
    """L, the left arrow, the word (letter by letter when its letters stand apart), the right arrow, R"""
    def split(cols):
        r = gaps_runs(cols)
        mid, letters = r[2:-2], word.replace(' ', '')
        toks = [('L',) + r[0], ('◀',) + r[1]]
        if len(mid) == len(letters):
            toks += [(c,) + m for c, m in zip(letters, mid)]
        else:
            toks.append((word, mid[0][0], mid[-1][1]))
        return toks + [('▶',) + r[-2], ('R',) + r[-1]]
    return split


def letter_tokens(text):
    """letter by letter where the cartridge spaced them out, else word by word"""
    def split(cols):
        r = gaps_runs(cols)
        letters = text.replace(' ', '')
        if len(r) == len(letters):
            return [(c,) + m for c, m in zip(letters, r)]
        words = runs(cols, len(text.split()))
        if not words:
            raise SystemExit(f"{text}: {len(r)} runs, {letters}")
        return [(t,) + m for t, m in zip(text.split(), words)]
    return split


def credit_tokens(text, idx):
    """the small "developed by" and the big "SQUARE ENIX / Jupiter", each fitted to its part"""
    small, big = text.split(' SQUARE')
    def split(cols):
        # SQUARE's S: the big letters' column with ink on rows 0-3, 5, 6 (the small words' accents reach row 0 too)
        sig = lambda x: tuple(bool(idx.getpixel((x, y))) for y in range(7))
        start = next(x for x in range(idx.width) if sig(x) == (True, True, True, True, False, True, True))
        on = [x for x, v in enumerate(cols) if v]
        a = [x for x in on if x < start]
        return [(small, a[0], a[-1] + 1), ('SQUARE' + big, start, on[-1] + 1)]
    return split


def draw(frame, kind, lang, src, idx, pal, text_rows, tokens, keep_x=None, bg=None, mono=False):
    w, h = src.size
    hd = Image.new('RGBA', (w * S, h * S), (0, 0, 0, 0))
    colour = lambda x, y: tuple(pal.get(str(idx.getpixel((x, y))), src.getpixel((x, y))[:3])) + (255,)
    text = [[False] * w for _ in range(h)]
    if tokens:
        y0, y1 = text_rows
        for y in range(y0, y1):
            for x in range(w):
                if idx.getpixel((x, y)) and (keep_x is None or keep_x(x, y)):
                    text[y][x] = True
    # the flat parts: rows of runs, colours interpolated along x between pixel centres (smooth gradients), crisp edges
    for y in range(h):
        x = 0
        while x < w:
            if not idx.getpixel((x, y)) or (text[y][x] and not bg):
                x += 1
                continue
            s = x
            while x < w and idx.getpixel((x, y)) and (bg or not text[y][x]):
                x += 1
            cs = [bg + (255,) if text[y][i] else colour(i, y) for i in range(s, x)]
            for px in range(s * S, x * S):
                u = (px + 0.5) / S - 0.5 - s
                i = min(max(int(u), 0), len(cs) - 1)
                j = min(i + 1, len(cs) - 1)
                c = lerp(cs[i], cs[j], min(max(u - i, 0), 1)) if 0 <= u else cs[0]
                for py in range(y * S, (y + 1) * S):
                    hd.putpixel((px, py), c)
    if bg:  # a banner: its silhouette smooth (the cartridge's stepped diagonals), its colour under the letters
        a = Image.new('L', (w, h), 0)
        for y in range(h):
            for x in range(w):
                if idx.getpixel((x, y)):
                    a.putpixel((x, y), 255)
        big = a.resize((w * S, h * S), Image.BILINEAR).filter(ImageFilter.GaussianBlur(S * 0.7))
        big = big.point(lambda v: max(0, min(255, (v - 128) * 4 + 128)))
        base = Image.new('RGBA', hd.size, bg + (255,))  # one colour: its stepped edge's shades were only the diagonal
        base.putalpha(big)
        hd = base
    if not tokens:
        return hd
    cols = [any(text[y][x] for y in range(h)) for x in range(w)]
    if callable(tokens):
        spans = tokens(cols)
    else:
        spans = runs(cols, len(tokens))
        if not spans:
            raise SystemExit(f'{frame}: fewer words than {tokens}')
        spans = [(t,) + r for t, r in zip(tokens, spans)]
    for tok, a, b in spans:
        ys = [y for y in range(h) for x in range(a, b) if text[y][x]]
        ty0, ty1 = min(ys), max(ys) + 1
        # its colour down the rows: the most common shown colour of each row's ink
        rowc = []
        for y in range(ty0, ty1):
            cs = {}
            for x in range(a, b):
                if text[y][x]:
                    c = colour(x, y)
                    cs[c] = cs.get(c, 0) + 1
            rowc.append(max(cs, key=cs.get) if cs else None)
        rowc = [c or next(d for d in rowc if d) for c in rowc]
        if bg or mono:  # letters of one colour (the shades were the pixel letters' edges)
            ink = [colour(x, y) for y in range(h) for x in range(w) if text[y][x]]
            rowc = [max(set(ink), key=ink.count)] * len(rowc)
        m = font_word(tok, (b - a) * S, (ty1 - ty0) * S)
        fill = Image.new('RGBA', m.size)
        for py in range(m.height):
            u = (py + 0.5) / S - 0.5
            i = min(max(int(u), 0), len(rowc) - 1)
            c = lerp(rowc[i], rowc[min(i + 1, len(rowc) - 1)], min(max(u - i, 0), 1)) if u >= 0 else rowc[0]
            for px in range(m.width):
                fill.putpixel((px, py), c[:3] + (255,))
        fill.putalpha(m)
        hd.alpha_composite(fill, (a * S, ty0 * S))
    return hd


def cursor(src, idx, pal, hd=None, top=0):
    """the orb (rows top..top+10): its rings (centre row, out from the middle) as a smooth rounded square"""
    w, h = src.size
    cy = top + 5
    xs = [x for x in range(min(w, 16)) if idx.getpixel((x, cy))]  # the orb's 16 columns, not the words after it
    cx0, cx1 = xs[0], xs[-1] + 1
    while idx.getpixel((cx1 - 1, cy)) != idx.getpixel((cx0, cy)):
        cx1 -= 1
    r = (cx1 - cx0) / 2
    mid = (cx0 + cx1) / 2
    prof = [src.getpixel((int(mid) + k, cy))[:3] for k in range(int(r) + 1)]
    if hd is None:
        hd = draw(None, None, None, src, idx, pal, None, None)
    for py in range(top * S, (top + 11) * S):
        for px in range(cx0 * S, cx1 * S):
            dx = abs((px + 0.5) / S - mid) / r
            dy = abs((py + 0.5) / S - top - 5.5) / 5.5
            d = (dx ** 4 + dy ** 4) ** 0.25
            if d > 1:
                continue
            t = d * (len(prof) - 0.5)
            i = min(int(t), len(prof) - 1)
            hd.putpixel((px, py), lerp(prof[i], prof[min(i + 1, len(prof) - 1)], t - i) + (255,))
    return hd


def main():
    pal = {}  # the decoded frames already carry the palettes title.c loads (title_words_src.py)
    OUT.mkdir(parents=True, exist_ok=True)
    rows = {}
    for v in ('eu', 'us'):
        for line in (SRC / v / 'index.tsv').read_text().splitlines():
            sheet, frame, x0, y0, w, h = line.split('\t')
            n = int(frame[frame.rindex('Frame') + 5:])
            src = Image.open(SRC / v / f'{frame}.png').convert('RGBA')
            idx = Image.open(SRC / v / f'{frame}.idx.png')
            if frame.startswith('gTitleMenuCursor'):
                hd = cursor(src, idx, pal.get(Sheet(frame), {}))
            elif frame.startswith('gTitleMenuSelectedEntry'):
                lang = lang_of(frame, 'gTitleMenuSelectedEntry')
                hd = draw(frame, 0, lang, src, idx, pal.get('gTitleMenuSelectedEntry', {}), (0, 9),
                          ENTRY[lang][n].split())
            elif frame.startswith('gTitleMenuEntry'):
                lang = lang_of(frame, 'gTitleMenuEntry')
                hd = draw(frame, 0, lang, src, idx, pal.get('gTitleMenuEntry', {}), (0, 9),
                          ENTRY[lang][n].split(), keep_x=lambda x, y: x >= 15)
            elif frame.startswith(('gTitleSoraLumi', 'gTitleRikuLumi')):
                lang = lang_of(frame, frame[:14])
                right = max(x for x in range(src.width) for y in range(2, 11) if idx.getpixel((x, y)) & 15 == 15)
                left = min(x for x in range(src.width) for y in range(2, 11) if idx.getpixel((x, y)) & 15 == 15)
                inked = [idx.getpixel((x, y)) for x in range(src.width) for y in range(2, 11) if idx.getpixel((x, y))]
                back = max(set(inked), key=inked.count)  # the banner's colour
                bx = next(x for x in range(src.width) if idx.getpixel((x, 6)) == back)
                hd = draw(frame, 0, lang, src, idx, {}, (2, 11), banner_tokens(LUMI[lang][n]),
                          keep_x=lambda x, y, r=right, l=left: l <= x <= r and idx.getpixel((x, y)) != back,
                          bg=src.getpixel((bx, 6))[:3])
            elif frame.startswith('gTitlePressStart'):
                lang = lang_of(frame, 'gTitlePressStart')
                line = next(y for y in range(src.height) if sum(1 for x in range(src.width) if idx.getpixel((x, y))) > src.width * 0.8)
                top = next(y for y in range(src.height) if idx.getpixel((8, y)))
                hd = draw(frame, 0, lang, src, idx, {}, (0, line), letter_tokens(PRESS[lang]),
                          keep_x=lambda x, y: x >= 15, mono=True)
                hd = cursor(src, idx, {}, hd, top)
            elif frame.startswith('gTitleDevelopedBy'):
                lang = lang_of(frame, 'gTitleDevelopedBy')
                hd = draw(frame, 0, lang, src, idx, {}, (0, 8), credit_tokens(CREDIT[lang], idx))
            else:
                continue
            hd.save(OUT / f'{frame}.png', optimize=True)
            rows.setdefault(frame, [sheet, frame, x0, y0, w, h, set()])[6].add(v)
    lines = ['/* HD title words (make_title_words.py): kHdSprites rows, group 7, pictures embedded in the game. */']
    for sheet, frame, x0, y0, w, h, regions in rows.values():
        cond = ' || '.join(f'defined({REGIONS[r]})' for r in sorted(regions))
        lines += [f'#if {cond}', f'    {{ {sheet}, {frame}, "{frame}", {x0}, {y0}, {w}, {h}, 7 }},', '#endif']
    INC.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    head = ['/* HD title words (make_title_words.py): the cartridge symbols title_words_hd.inc matches. */']
    for sheet, frame, x0, y0, w, h, regions in rows.values():
        cond = ' || '.join(f'defined({REGIONS[r]})' for r in sorted(regions))
        head += [f'#if {cond}', f'extern const unsigned char {sheet}[];', f'extern const unsigned char {frame}[];',
                 '#endif']
    INC.with_suffix('.h').write_text('\n'.join(head) + '\n', encoding='utf-8')
    print(len(rows), 'words ->', OUT)


if __name__ == '__main__':
    main()
