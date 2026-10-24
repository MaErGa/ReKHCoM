"""The newest review submission with clouds for 'Escena NNN' (or a title prefix): its comments, and its marked image
at a third of the size in the session scratchpad (sub-<NNN>.png). usage: python build/show_sub.py <title prefix>"""
import json
import sys
from pathlib import Path
from PIL import Image

DATA = Path(r'C:/Users/MP/Documents/00 Claude/KHCoMPC/khcom-codex/build/art-review')
OUT = Path(r'C:/Users/MP/AppData/Local/Temp/claude/C--Users-MP-Documents-00-Claude-KHCoMPC/04c98b95-f93a-4587-8f6d-ef3665d55d5f/scratchpad')
want = sys.argv[1]
if want.isdigit():
    want = 'Escena %03d' % int(want)
best = None
for path in sorted((DATA / 'envios').glob('*/comentarios.json')):
    payload = json.loads(path.read_text(encoding='utf-8'))
    for a in payload['assets']:
        if a['title'].startswith(want) and a['annotations']:
            best = (path.parent, a)
d, a = best
print(a['title'], a['id'], d.name, a['width'], a['height'])
print('general:', a['general'])
for n in a['annotations']:
    print(' ', n['number'], n['pixels_xyxy_exclusive'], n['comment'])
im = Image.open(d / a['marked'])
name = 'sub-' + ''.join(c for c in want if c.isalnum()) + '.png'
im.resize((im.width // 3, im.height // 3)).save(OUT / name)
print(OUT / name)
