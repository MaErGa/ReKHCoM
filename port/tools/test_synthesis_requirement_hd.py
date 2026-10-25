#!/usr/bin/env python3
"""Prepare an isolated serial synthesis capture. Default NEVER runs the game.

After the coordinator permits runtime testing, add --run. Optional --baseline
compares HD-off pixels against a previous executable directory. This smoke
fixture covers the real first door (1 >=); it does not claim all-rule coverage.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

from PIL import Image
from synthesis_requirement_hd import ASSETS, ROOT, specs

FRAMES = (300, 380, 470, 510)


def prepare(build, out, hd_source, enabled):
    out.mkdir(parents=True, exist_ok=False)
    for name in ('khcom.exe', 'SDL3.dll'):
        shutil.copy2(build / name, out / name)
    shutil.copytree(ROOT / 'port/assets/fonts', out / 'assets/fonts')
    sprites = out / 'hd/sprites'
    if hd_source:
        shutil.copytree(hd_source, sprites)
    else:
        sprites.mkdir(parents=True)
    for _, name, _, _, _ in specs():
        shutil.copy2(ASSETS / (name + '.png'), sprites / (name + '.png'))
    (out / 'kh_com.ini').write_text(
        f'config_version=11\nview_mode=4\nzoom=8\nhd_text=1\nhd_cards={enabled}\nvsync=0\n',
        encoding='utf-8')
    env = dict(SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1',
               KHCOM_HD_SPRITES=str(sprites.resolve()), KHCOM_WINDOW='1920x1080',
               KHCOM_CONFIG=f'hd_cards={enabled};hd_text=1',
               KHCOM_MODE='60:room:10901,120:fieldfreeze,200:doorpose:0',
               KHCOM_KEYS='240-242:A', KHCOM_HDDRAWLOG='470',
               KHCOM_OUTPUT_SHOT=','.join(map(str, FRAMES)), KHCOM_QUIT='512')
    command = [str((out / 'khcom.exe').resolve()), '--language', 'es', '--rom',
               str((ROOT / 'roms/B8CP.gba').resolve())]
    (out / 'capture.json').write_text(json.dumps(dict(command=command, env=env), indent=2),
                                    encoding='utf-8')
    return command, env


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True, help='New directory; never overwritten')
    parser.add_argument('--hd', type=Path, help='Existing sprites to merge with synthesis pieces')
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--run', action='store_true', help='Explicitly launch serial captures')
    args = parser.parse_args()
    jobs = []
    for name, build, enabled in [('hd-on', args.build, 1), ('hd-off', args.build, 0)]:
        folder = args.out / name
        jobs.append((name, folder, *prepare(build, folder, args.hd, enabled)))
    if args.baseline:
        folder = args.out / 'baseline-off'
        jobs.append(('baseline-off', folder, *prepare(args.baseline, folder, args.hd, 0)))
    if not args.run:
        print(f'Prepared {len(jobs)} isolated captures in {args.out}; NO game launched')
        return
    for name, folder, command, extra in jobs:
        env = {k: v for k, v in os.environ.items() if not k.startswith(('KHCOM_', 'SDL_'))}
        env.update(extra)
        subprocess.run(command, cwd=folder, env=env, check=True, timeout=180,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        log = (folder / 'kh_com.log').read_text(errors='replace')
        assert 'CRASH' not in log and 'outside its box' not in log, name
        if name == 'hd-on':
            for asset in ('synthesis_body_none', 'synthesis_at_least_01'):
                assert f'hd draw 470 {asset} ' in log, (name, asset)
        else:
            assert 'hd draw 470 synthesis_' not in log, name
    if args.baseline:
        for frame in FRAMES:
            with Image.open(args.out / 'hd-off' / f'out_{frame:05d}.bmp') as actual, \
                 Image.open(args.out / 'baseline-off' / f'out_{frame:05d}.bmp') as before:
                assert actual.size == before.size and actual.tobytes() == before.tobytes(), frame
    print('PASS serial first-door capture; inspect HD-on screenshots before Atlas submission')


if __name__ == '__main__':
    main()
