"""Export the native completion with the game's animation order and timing."""
import json
from pathlib import Path
import re

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'port/art/proposals/maleficent-continuity'
audit = json.loads((ROOT / 'build/maleficent-native-audit/manifest.json').read_text())
manifest = json.loads((OUT / 'manifest.json').read_text())
palette = manifest['palette']
background = (216, 216, 223)
colours = palette + [background] + [(0, 0, 0)] * 239
flat_palette = [channel for colour in colours for channel in colour]
fixed = Image.new('P', (1, 1))
fixed.putpalette(flat_palette)
poses = {}
for group in manifest['groups']:
    layer = Image.frombytes('P', (512, 384), (OUT / (group['cut'] + '.indices')).read_bytes())
    layer.putpalette(flat_palette)
    layer.info['transparency'] = 0
    for slot in group['frames']:
        native = Image.open(audit['frames'][slot]['mounted']).convert('RGBA')
        complete = layer.convert('RGBA')
        complete.alpha_composite(native)
        poses[slot] = complete

source = (ROOT / 'src/bos/bos_md.c').read_text()
sequences = {}
for name, contents in re.findall(r'static const MdAnimFrame (\w+)\[\d+\] = \{(.*?)\};', source):
    sequences[name] = [tuple(map(int, p)) for p in re.findall(r'\{\s*(\d+)\s*,\s*(\d+)\s*\}', contents)]
names = ['sMdIdleFrames'] * 2 + [
    'sMdFireBreathStartFrames', 'sMdFireBreathFrames', 'sMdFireBreathFrames',
    'sMdFireBreathEndFrames', 'sMdIdleFrames', 'sMdQuakeDoubleFrames',
    'sMdIdleFrames', 'sMdQuakeHeavyFrames', 'sMdIdleFrames',
    'sMdBiteNearFrames', 'sMdBiteFarFrames', 'sMdBiteDoubleFrames', 'sMdIdleFrames',
]
timeline = [pair for name in names for pair in sequences[name]]
boxes = [poses[slot].getbbox() for slot, ticks in timeline]
crop = (max(0, min(b[0] for b in boxes) - 8), max(0, min(b[1] for b in boxes) - 8),
        min(512, max(b[2] for b in boxes) + 8), min(384, max(b[3] for b in boxes) + 8))
frames, durations = [], []
elapsed, previous_ms = 0, 0
for slot, ticks in timeline:
    canvas = Image.new('RGBA', (512, 384), background + (255,))
    canvas.alpha_composite(poses[slot])
    frame = canvas.crop(crop).convert('RGB')
    frame = frame.resize((frame.width * 3, frame.height * 3), Image.Resampling.NEAREST)
    frames.append(frame.quantize(palette=fixed, dither=Image.Dither.NONE))
    # MdAnimUpdate advances only when --timer < 0: each entry lasts duration+1.
    elapsed += ticks + 1
    milliseconds = round(elapsed * 1000 / 60 / 10) * 10
    durations.append(milliseconds - previous_ms)
    previous_ms = milliseconds
path = OUT / 'malefica-nueva-animada.gif'
frames[0].save(path, save_all=True, append_images=frames[1:], duration=durations,
               loop=0, disposal=2, optimize=False)
with Image.open(path) as gif:
    assert gif.is_animated and gif.n_frames > 20
    actual_duration = sum(gif.seek(i) or gif.info['duration'] for i in range(gif.n_frames))
    assert actual_duration == sum(durations)
report = dict(file=path.name, animations=names, timeline=timeline, crop=crop,
              scale=3, fps=60, duration_ms=actual_duration, palette='native16 + neutral background',
              note='Offline assembly of native poses and completion; original order and frame durations, not a gameplay video.')
(OUT / 'animation-replay.json').write_text(json.dumps(report, indent=2))
print(path)
print('PASS:', len(frames), 'poses on timeline,', actual_duration, 'ms,', path.stat().st_size, 'bytes')
