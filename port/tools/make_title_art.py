"""The HD title's illustration (mod): the CoM poster the user supplied, its white paper made transparent and its edges
faded, so it lies on the title's own white (and its fades) without a seam. PIL only."""
from pathlib import Path
from PIL import Image, ImageChops

SRC = r'C:\Users\MP\Documents\00 Claude\KHCoMPC\x5zh3428mzyf1.jpg'
OUT = Path(r'C:\Users\MP\Documents\00 Claude\KHCoMPC\khcom-recom-art\port\assets\hd\title\art\sora.png')
PREVIEW = r'D:\01Claude\ReKHCoM-build\titlehd\art_check.png'

im = Image.open(SRC).convert('RGB')
w, h = im.size
r, g, b = im.split()
low = ImageChops.darker(ImageChops.darker(r, g), b)
# near-white paper -> transparent; anything with ink -> opaque
paper = low.point(lambda v: max(0, min(255, (255 - v) * 5)))


def smooth(t):
    t = max(0.0, min(1.0, t))
    return t * t * (3 - 2 * t)


def ramp_mask(size, horizontal, start, end, reverse=False):
    n = size[0] if horizontal else size[1]
    line = [int(255 * smooth((i if not reverse else n - 1 - i) / max(1, end))) for i in range(n)]
    strip = Image.new('L', (n, 1) if horizontal else (1, n))
    strip.putdata(line)
    return strip.resize(size, Image.NEAREST)


mask = paper
for m in (ramp_mask((w, h), False, 0, h * 0.06),             # top: the cut panels fade in
          ramp_mask((w, h), False, 0, h * 0.07, True),       # bottom
          ramp_mask((w, h), True, 0, w * 0.30),              # left: Goofy's and Jiminy's panels go under the logo
          ramp_mask((w, h), True, 0, w * 0.05, True)):       # right
    mask = ImageChops.multiply(mask, m)
out = im.copy()
out.putalpha(mask)
OUT.parent.mkdir(parents=True, exist_ok=True)
out.save(OUT, optimize=True)
print(OUT, out.size, OUT.stat().st_size)

pw, ph = w // 3, h // 3
white = Image.new('RGBA', (w, h), (255, 255, 255, 255)); white.alpha_composite(out)
dark = Image.new('RGBA', (w, h), (40, 40, 60, 255)); dark.alpha_composite(out)
sheet = Image.new('RGB', (pw * 2, ph))
sheet.paste(white.convert('RGB').resize((pw, ph)), (0, 0))
sheet.paste(dark.convert('RGB').resize((pw, ph)), (pw, 0))
sheet.save(PREVIEW)
