"""Hidden, silent moving-camera Jafar regression captures and per-frame PPU log."""
import argparse
import os
import re
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--frames', type=int, default=1800)
    ap.add_argument('--size', default='1920x1080')
    ap.add_argument('--baseline', action='store_true', help='capture old bug without asserting the fix')
    args = ap.parse_args()
    exe = args.exe.resolve()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    shutil.copy2(exe, out / 'khcom.exe')
    shutil.copy2(exe.parent / 'SDL3.dll', out / 'SDL3.dll')
    (out / 'kh_com.ini').write_text('config_version=11\nview_mode=4\nzoom=6\nhd_text=0\nvsync=0\n')
    keys = '60-63:A,' + ','.join(f'{f}-{f+70}:{"RIGHT" if (f//100)%2 else "LEFT"}'
                                  for f in range(300, max(1800, args.frames)-80, 100))
    shots = '240,500,512,513,514,515,900,1200,1500'
    env = dict(os.environ, SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy',
               KHCOM_FAST='1', KHCOM_WINDOW=args.size, KHCOM_BATTLE='204:1:0',
               KHCOM_KEYS=keys, KHCOM_QUIT=str(args.frames), KHCOM_OUTPUT_SHOT=shots,
               KHCOM_PPULOG='1', KHCOM_REGLOG=f'0-{args.frames}', KHCOM_PPU_THREADS='1')
    with (out / 'stdout.txt').open('w') as log:
        result = subprocess.run([str(out / 'khcom.exe'), '--rom', str(ROOT / 'roms/B8CP.gba')],
                                cwd=out, env=env, stdout=log, stderr=log, timeout=600,
                                creationflags=getattr(subprocess, 'BELOW_NORMAL_PRIORITY_CLASS', 0))
    if result.returncode:
        raise SystemExit(f'Jafar exited {result.returncode}: {out}')
    text = (out / 'kh_com.log').read_text(errors='replace')
    rows = re.findall(r'reg (\d+):[^\n]*\nppu: dispcnt ([0-9a-f]+) latch ([01]{4})[^\n]* wide ([01])', text)
    active = [(int(f), latch, wide) for f, cnt, latch, wide in rows
              if int(f) >= 100 and int(cnt, 16) & 0x100 and not int(cnt, 16) & 0x80]
    bad = [f for f, latch, wide in active if latch[0] != '1' or wide != '1']
    print(f'{len(active)} active room frames, {len(bad)} lost room/view: {bad[:20]}')
    if not args.baseline and (not active or bad or int(rows[-1][0]) < args.frames):
        raise SystemExit(f'Jafar room/view regression: {out}')
    print(out)


if __name__ == '__main__':
    main()
