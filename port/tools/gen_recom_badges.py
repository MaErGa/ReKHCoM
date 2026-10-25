"""Re:CoM achievement art (approved 25/10, art review 0d49fe5c): a filigree border in the manner of Re:Chain of Memories' (PS2) menus (gold
scrollwork, a crown at the top), drawn around the 64x64 badges; plus the mode's own badges built from the set's."""
import math
from PIL import Image, ImageDraw, ImageFilter

S = 8  # supersampling
OUT = 80  # framed badge size (badge 64 + 8 px each side)

def frame_layer():
    W = OUT * S
    im = Image.new('RGBA', (W, W), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    gold_dark, gold, gold_light = (122, 84, 20, 255), (205, 160, 52, 255), (255, 232, 150, 255)
    o = 2 * S
    # outer and inner rails
    d.rounded_rectangle((o, o, W - o, W - o), radius=6 * S, outline=gold_dark, width=3 * S)
    d.rounded_rectangle((o + S, o + S, W - o - S, W - o - S), radius=5 * S, outline=gold, width=2 * S)
    i = 7 * S
    d.rectangle((i, i, W - i, W - i), outline=gold_dark, width=S)
    d.rectangle((i + S, i + S, W - i - S, W - i - S), outline=gold_light, width=S // 2)
    # corner scrolls: a spiral curl at each corner
    def curl(cx, cy, sx, sy):
        pts = []
        for k in range(0, 260):
            t = k / 259 * 2.6 * math.pi
            r = (7.5 - 2.4 * t / math.pi) * S
            pts.append((cx + sx * r * math.cos(t), cy + sy * r * math.sin(t)))
        d.line(pts, fill=gold_dark, width=int(2.6 * S))
        d.line(pts, fill=gold, width=int(1.4 * S))
        d.ellipse((cx - 1.6 * S, cy - 1.6 * S, cx + 1.6 * S, cy + 1.6 * S), fill=gold_light)
    c = 9 * S
    for cx, cy, sx, sy in ((c, c, 1, 1), (W - c, c, -1, 1), (c, W - c, 1, -1), (W - c, W - c, -1, -1)):
        curl(cx, cy, sx, sy)
    # small leaves along the sides
    for k in (0.33, 0.67):
        for (x, y, horiz) in ((W * k, o + 1.5 * S, True), (W * k, W - o - 1.5 * S, True),
                              (o + 1.5 * S, W * k, False), (W - o - 1.5 * S, W * k, False)):
            if horiz:
                d.ellipse((x - 3 * S, y - 1.2 * S, x + 3 * S, y + 1.2 * S), fill=gold, outline=gold_dark, width=S // 2)
            else:
                d.ellipse((x - 1.2 * S, y - 3 * S, x + 1.2 * S, y + 3 * S), fill=gold, outline=gold_dark, width=S // 2)
    # the crown at the top middle
    cx, top = W / 2, 0.3 * S
    crown = [(cx - 7 * S, top + 7 * S), (cx - 7 * S, top + 2.5 * S), (cx - 3.5 * S, top + 5 * S), (cx, top),
             (cx + 3.5 * S, top + 5 * S), (cx + 7 * S, top + 2.5 * S), (cx + 7 * S, top + 7 * S)]
    d.polygon(crown, fill=gold, outline=gold_dark)
    d.line(crown + [crown[0]], fill=gold_dark, width=S)
    for px in (cx - 7 * S, cx, cx + 7 * S):
        y = top + (2.5 * S if px != cx else 0)
        d.ellipse((px - 1.3 * S, y - 1.3 * S, px + 1.3 * S, y + 1.3 * S), fill=gold_light, outline=gold_dark)
    return im.resize((OUT, OUT), Image.LANCZOS)

def badge(atlas, index, grey=False):
    b = atlas.crop((index % 10 * 64, index // 10 * 64, index % 10 * 64 + 64, index // 10 * 64 + 64)).convert('RGBA')
    if grey:
        b = b.convert('L').convert('RGBA')
    return b

def framed(b, frame):
    out = Image.new('RGBA', (OUT, OUT), (0, 0, 0, 0))
    out.paste(b, (8, 8))
    out.alpha_composite(frame)
    return out

def pips(b, n, colour=(255, 222, 120, 255)):
    """the mode's own: the story's badge with its difficulty as 1-3 gold stars along the bottom"""
    b = b.copy()
    d = ImageDraw.Draw(b)
    d.rectangle((0, 50, 63, 63), fill=(20, 16, 30, 200))
    for k in range(n):
        x = 32 + (k - (n - 1) / 2) * 13
        star = [(x + 5 * math.cos(math.pi / 2 + j * math.pi / 5) * (1 if j % 2 == 0 else 0.45),
                 57 - 5 * math.sin(math.pi / 2 + j * math.pi / 5) * (1 if j % 2 == 0 else 0.45)) for j in range(10)]
        d.polygon(star, fill=colour, outline=(120, 84, 20, 255))
    return b

ROW = 9  # the atlas row after the 90 official badges (10 per row)


def main():
    """Rewrite row 9 of port/assets/achievements/badges.png: cells 0-6 the mode's own badges (in RECOM_* order:
    Sora 1-3 stars, Riku 1-3, Memory Master), cell 7 the filigree frame (80 px drawn into 64: shown 1.25x the badge)."""
    from pathlib import Path
    src = Path(__file__).resolve().parents[2] / 'port/assets/achievements/badges.png'
    atlas = Image.open(src).convert('RGBA')
    official = atlas.crop((0, 0, 640, ROW * 64))
    sora, riku = 21, 40  # "Memory of Her" (Sora's story) and "The Road to Dawn" (Reverse/Rebirth)
    own = [pips(badge(official, sora), n) for n in (1, 2, 3)] + [pips(badge(official, riku), n) for n in (1, 2, 3)]
    half = badge(official, sora).copy()
    half.paste(badge(official, riku).crop((32, 0, 64, 64)), (32, 0))
    own.append(pips(half, 3, (255, 240, 200, 255)))
    out = Image.new('RGBA', (640, (ROW + 1) * 64), (0, 0, 0, 0))
    out.paste(official, (0, 0))
    for k, b in enumerate(own):
        out.paste(b, (k * 64, ROW * 64))
    out.paste(frame_layer().resize((64, 64), Image.LANCZOS), (7 * 64, ROW * 64))
    out.save(src, optimize=True)
    print(src, out.size)


if __name__ == '__main__':
    main()
