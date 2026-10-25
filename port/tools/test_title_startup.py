"""Check optional-asset discovery finishes before title playback (EU + optional JP ROM).

Runs serially, hidden and silent; --gpu measures real D3D presentation at normal speed.
No ROM, save or configuration outside the test folder is modified.
"""
import argparse
import csv
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--build', type=Path, default=ROOT / 'build/pc-eu')
    ap.add_argument('--jp-rom', type=Path, default=ROOT / 'roms/B8CJ.gba')
    ap.add_argument('--out', type=Path, default=ROOT / 'build/title-startup')
    ap.add_argument('--gpu', action='store_true')
    args = ap.parse_args()
    if not args.jp_rom.is_file():
        ap.error('the JP ROM is required to exercise the title CRC regression; pass --jp-rom')
    for present in (False, True):
        d = args.out.resolve() / ('with-jp' if present else 'without-jp')
        d.mkdir(parents=True, exist_ok=True)
        for name in ('khcom.exe', 'SDL3.dll'):
            shutil.copy2(args.build / name, d / name)
        (d / 'roms').mkdir(exist_ok=True)
        jp = d / 'roms/B8CJ.gba'
        if present and not jp.exists():
            try:
                os.link(args.jp_rom.resolve(), jp)
            except OSError:
                shutil.copy2(args.jp_rom, jp)
        (d / 'kh_com.ini').write_text('config_version=11\ngraphics_mode=2\ngraphics_mode_applied=2\n'
            'view_mode=4\nzoom=6\nhd_text=1\nhd_movies=0\nhd_scenes=0\nhd_music=0\nhd_hud=0\n'
            'undub=0\nvsync=0\nexperimental_frame_skip=0\n')
        env = {k: v for k, v in os.environ.items() if not k.startswith('KHCOM_')}
        env.update(SDL_AUDIO_DRIVER='dummy', KHCOM_PPU_THREADS='1', KHCOM_BACKGROUND_TEST='1',
            KHCOM_HIDDEN='1', KHCOM_WINDOW='1920x1080', KHCOM_QUIT='760', KHCOM_KEYS='30-33:A',
            KHCOM_FRAMELOG=str(d / 'frames.csv'))
        if not args.gpu:
            env.update(SDL_VIDEO_DRIVER='dummy', KHCOM_FAST='1')
        with (d / 'stdout.txt').open('w') as output:
            subprocess.run([str(d / 'khcom.exe'), '--rom', str(ROOT / 'roms/B8CP.gba')], cwd=d,
                env=env, timeout=60, check=True, stdout=output, stderr=output,
                creationflags=getattr(subprocess, 'BELOW_NORMAL_PRIORITY_CLASS', 0))
        log = (d / 'kh_com.log').read_text(errors='replace')
        checked = log.index('startup: optional assets checked')
        title = re.search(r'mode: mode_title \(frame (\d+)\)', log)
        assert title and checked < title.start(), 'asset scan must precede the visible title'
        assert not re.search(r'CRASH|FATAL', log), 'game reported a crash'
        rows = list(csv.DictReader((d / 'frames.csv').open()))
        frame = int(title[1])
        transition = [float(r['interval_ms']) for r in rows if frame <= int(r['frame']) <= frame + 8]
        assert transition and int(rows[-1]['frame']) >= 760, 'title playback did not finish'
        print(f'{"with" if present else "without"} JP: scan before title; transition max {max(transition):.2f} ms')


if __name__ == '__main__':
    main()
