#!/usr/bin/env python3
"""Exercise the real paused options menu; isolated copies, serial game runs.

The dummy video driver tests menu changes and rendering, not monitor mode
switching or physical VSync. Reports explicitly retain that limitation.
"""
import argparse
import datetime
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]


# The rows below were written for the options' old order (picture first); 24/10 the game's rows moved first. They are
# translated by name, so the route still exercises the same settings.
OLD_ORDER = ['VIEW', 'ASPECT', 'ZOOM', 'INTZOOM', 'SCALE', 'HUD', 'NARROW', 'FILTER', 'FULLSCREEN', 'FS_RES', 'VSYNC',
             'SCANLINES', 'LCD', 'CARDS', 'FPS', 'DISCORD', 'DODGE', 'STOCK', 'SAVEBANK', 'UNDUB', 'MUSIC', 'SFX', 'MASTER']


def item_numbers():
    src = (ROOT / 'port/src/port_ui.c').read_text(encoding='utf-8')
    body = src[src.index('enum {\n    /* the game'):]
    body = body[:body.index('};')]
    return {m: i for i, m in enumerate(re.findall(r'^\s*ITEM_(\w+)\s*[,=]', body, re.M))}


NEW = item_numbers()
TO_NEW = {old: NEW[name] for old, name in enumerate(OLD_ORDER)}
TO_OLD = {new: old for old, new in TO_NEW.items()}


def route():
    actions = []
    def change(row, count=1, direction=1):
        for _ in range(count):
            actions.append((len(actions) * 3 + 3, TO_NEW.get(row, row), direction))
    # Auto zoom, then disabled scaling; original scaling, then disabled zoom.
    change(1, 3)
    change(1, 3, -1)
    change(2)
    change(0)
    change(1)
    change(2, 3)
    change(0, 4)
    for row, count in ((3, 3), (4, 2), (5, 3)):
        change(row, count)
    change(6)  # resolution changes happen while the window is fullscreen
    change(7, 2)
    change(6)
    for row, count in ((8, 2), (9, 5), (10, 2), (11, 5), (12, 2),
                       (13, 2), (14, 2), (15, 2), (16, 10),
                       (17, 10), (18, 10), (19, 10), (21, 2),
                       (22, 2), (23, 2)):
        change(row, count, -1 if row in (17, 18, 19) else 1)
    return actions


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--versions', nargs='+', choices=('eu', 'us', 'jp'), default=['eu', 'us', 'jp'])
    parser.add_argument('--languages', nargs='+', default=['en', 'fr', 'de', 'it', 'es'])
    parser.add_argument('--build-prefix', default='build/codex-text-options-')
    parser.add_argument('--timeout', type=int, default=120)
    parser.add_argument('--video', choices=('dummy', 'windows'), default='dummy',
                        help='windows opens an isolated real game window and exercises fullscreen')
    args = parser.parse_args()
    base = ROOT / 'build/codex-text-options-results' / datetime.datetime.now().strftime('%Y%m%d_%H%M%S_%f')
    base.mkdir(parents=True)
    actions = route()
    results = []
    for version in args.versions:
        for lang in args.languages if version == 'eu' else ['ja' if version == 'jp' else 'en']:
            work = base / f'{version}-{lang}'
            work.mkdir()
            for filename in ('khcom.exe', 'SDL3.dll'):
                shutil.copy2(ROOT / (args.build_prefix + version) / filename, work / filename)
            shutil.copytree(ROOT / 'port/assets/fonts', work / 'assets/fonts')
            (work / 'kh_com.ini').write_text('config_version=2\nview_mode=4\nhd_text=0\nwindow_scale=2\nfullscreen_width=0\nfullscreen_height=0\n')
            env = {key: value for key, value in os.environ.items() if not key.startswith('KHCOM_')}
            env.update(SDL_VIDEO_DRIVER=args.video, SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1',
                       KHCOM_OPTIONS_TOGGLE='100', KHCOM_OPTIONS_HOLD=str(actions[-1][0] + 10),
                       KHCOM_OPTIONS_ACTIONS=','.join(f'{tick}:{row}:{direction}' for tick, row, direction in actions),
                       KHCOM_QUIT='180', KHCOM_TRACE_MODES='1', KHCOM_OUTPUT_SHOT='175')
            rom = {'eu': 'B8CP', 'us': 'B8CE', 'jp': 'B8CJ'}[version]
            try:
                proc = subprocess.run([str(work / 'khcom.exe'), '--language', lang, '--rom',
                                       str(ROOT / 'roms' / f'{rom}.gba')], cwd=work, env=env,
                                      stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=args.timeout)
                code = proc.returncode
            except subprocess.TimeoutExpired:
                code = 'timeout'
            log = (work / 'kh_com.log').read_text(errors='replace')
            found = re.findall(r'options-test: tick=(\d+) item=(\d+) enabled=(\d+) before=\[(.*?)\] after=\[(.*?)\]', log)
            rows = {TO_OLD.get(int(item), int(item)) for _, item, _, _, _ in found}
            pics = list(work.glob('options_*.bmp'))
            expected_rows = set(range(24)) - {20}
            disabled = {(TO_OLD.get(int(row), int(row)), enabled) for _, row, enabled, before, after in found if before == after}
            passed = code == 0 and 'CRASH' not in log and len(found) == len(actions) and len(pics) == len(actions)
            passed &= rows == expected_rows and (1, '0') in disabled and (2, '0') in disabled
            passed &= (work / 'out_00175.bmp').exists()
            config = dict(line.split('=', 1) for line in (work / 'kh_com.ini').read_text().splitlines()
                          if '=' in line and not line.startswith('#'))
            expected_config = dict(view_mode='4', zoom='0', scale_mode='0', filter='0', fullscreen='0',
                                   vsync='1', hud_size='1', narrow_fit='1', scanlines='0', color_correction='0',
                                   show_fps='0', master_volume='0', music_volume='0', sfx_volume='0',
                                   dodge_button='1', stock_button='1', unstock_button='1', save_bank='1',
                                   hd_text='0', card_hud_raise='8', hd2d='0', smooth_audio='0')
            persisted = all(config.get(key) == value for key, value in expected_config.items())
            passed &= persisted
            video = re.findall(r'options-video: tick=(\d+) fullscreen=(\d+) size=(\d+)x(\d+) mode=(\d+)x(\d+) vsync=(-?\d+) driver=(\w+)', log)
            hardware = args.video == 'windows' and {row[1] for row in video} == {'0', '1'} and {row[6] for row in video} >= {'0', '1'}
            if args.video == 'windows':
                passed &= hardware
            result = dict(version=version, language=lang, video=args.video, exit=code, actions=len(found), shots=len(pics),
                          rows=sorted(rows), passed=bool(passed), directory=str(work),
                          persisted=persisted, video_state=video, hardware_switches=hardware,
                          limitations=(['dummy driver: physical fullscreen/display modes and VSync not verified'] if args.video == 'dummy' else []) +
                                      ['menu mutation/persistence/rendering; gameplay effects require scene tests'])
            (work / 'result.json').write_text(json.dumps(result, indent=2))
            print(json.dumps(result), flush=True)
            results.append(result)
    (base / 'results.json').write_text(json.dumps(results, indent=2))
    print(base)
    return 0 if all(result['passed'] for result in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
