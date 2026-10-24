"""Options needing assets, the new game's toggles, the new-asset offer and the big confirmations (24/10).

Runs build/pc/khcom.exe in isolated folders (build/assets_ui/<case>), no window, no sound. Usage:
  python port/tools/test_asset_options.py [case ...]     cases: notice, newgame, confirm_mode, confirm_title, title_last
Prints each case's log lines and kh_com.ini values; the pictures stay in each folder (wizard_*.bmp, options_*.bmp).
"""
import os, re, shutil, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EXE = ROOT / 'build/pc/khcom.exe'
OUT = ROOT / 'build/assets_ui'


def items():
    """the ITEM_* numbers, from port_ui.c's enum"""
    src = (ROOT / 'port/src/port_ui.c').read_text(encoding='utf-8')
    body = src[re.search(r'enum \{\s*(/\*.*?\*/\s*)?ITEM_', src, re.S).start():]  # a comment may open the enum
    body = body[:body.index('};')]
    names, n = {}, 0
    for m in re.finditer(r'^\s*(ITEM_\w+)(\s*=\s*([^,]+))?,', body, re.M):
        if m.group(3):
            base, _, add = m.group(3).partition('+')
            n = names[base.strip()] + int(add or 0)
        names[m.group(1)] = n
        n += 1
    return names


def run(case, ini, env, files=(), frames=900, roms=('B8CP.gba', 'B8CJ.gba')):
    d = OUT / case
    shutil.rmtree(d, ignore_errors=True)
    d.mkdir(parents=True)
    for f in ('khcom.exe', 'SDL3.dll'):
        shutil.copy(ROOT / 'build/pc' / f, d)
    (d / 'roms').mkdir()
    for rom in roms:
        shutil.copy(ROOT / 'roms' / rom, d / 'roms')
    (d / 'kh_com.ini').write_text(''.join(f'{k}={v}\n' for k, v in ini.items()), encoding='utf-8')
    for f in files:
        p = d / f
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(b'')
    e = dict(os.environ, SDL_AUDIO_DRIVER='dummy', SDL_VIDEO_DRIVER='offscreen', KHCOM_FAST='1',
             KHCOM_QUIT=str(frames), KHCOM_WATCHDOG='120', KHCOM_TRACE_MODES='1', **env)
    try:
        r = subprocess.run([str(d / 'khcom.exe')], cwd=d, env=e, timeout=300)
    except subprocess.TimeoutExpired:
        subprocess.run(['taskkill', '/F', '/IM', 'khcom.exe', '/FI', f'WINDOWTITLE ne x'], capture_output=True) if False else None
        r = subprocess.CompletedProcess([], 'timeout')
    log = (d / 'kh_com.log').read_text(encoding='utf-8', errors='replace') if (d / 'kh_com.log').exists() else ''
    out = {}
    for line in (d / 'kh_com.ini').read_text(encoding='utf-8').splitlines():
        k, _, v = line.partition('=')
        out[k] = v
    print(f'== {case}: exit {r.returncode}')
    for line in log.splitlines():
        if re.search(r'wizard-test|confirm-test|options-test|crash|fault|bios:|harness: frame', line):
            print('  ', line)
    return out, d


ROUTE = '150-152:DOWN,165-167:DOWN,180-182:DOWN,200-204:A,1260-1264:START,1310-1314:A,1360-1364:A,1410-1414:A,1460-1464:A,1520-1524:A,1580-1584:A,1640-1644:A,1700-1704:A'
BASE = {'config_version': 11, 'graphics_mode': 2, 'graphics_mode_applied': 2, 'game_language': 0}


def main(cases):
    it = items()
    if 'notice' in cases:  # HD music imported since the last start, and the Japanese ROM: offered once, accepted
        ini = dict(BASE, game_language=5, assets_seen=0, hd_music=0, undub=0)
        got, d = run('notice', ini, {'KHCOM_MODE_CHOICE': '1', 'KHCOM_WIZARD_KEYS': '30:1,32:1,34:3',
                                     'KHCOM_WIZARD_SHOTS': '20'}, files=['hd/music/Title.ogg'], frames=1600)
        print('   ini: hd_music', got.get('hd_music'), 'undub', got.get('undub'), 'assets_seen', got.get('assets_seen'),
              'pictures', sorted(p.name for p in d.glob('wizard_*')))
        # only the HD music (no Japanese ROM): the single window, its title naming it
        got1, d1 = run('notice_one', dict(ini), {'KHCOM_MODE_CHOICE': '1', 'KHCOM_WIZARD_KEYS': '30:3',
                                                'KHCOM_WIZARD_SHOTS': '20'}, files=['hd/music/Title.ogg'], frames=1600,
                       roms=('B8CP.gba',))
        print('   one: hd_music', got1.get('hd_music'), 'assets_seen', got1.get('assets_seen'))
        # the next start: nothing new, no window
        ini = dict(BASE, game_language=5, assets_seen=got.get('assets_seen'), hd_music=0, undub=0)
        got, d = run('notice_again', ini, {'KHCOM_MODE_CHOICE': '1', 'KHCOM_WIZARD_KEYS': '30:3',
                                           'KHCOM_WIZARD_SHOTS': '20'}, files=['hd/music/Title.ogg'], frames=1600)
        print('   again: hd_music', got.get('hd_music'), 'pictures', sorted(p.name for p in d.glob('wizard_*')))
    if 'notice_many' in cases:  # HD HUD, music and subtitles imported at once, and the Japanese ROM: a switch each
        ini = dict(BASE, game_language=5, assets_seen=0, hd_music=0, hd_cards=0, hd_title=0, hd_subtitles=0, undub=0)
        got, d = run('notice_many', ini, {'KHCOM_MODE_CHOICE': '1', 'KHCOM_WIZARD_SHOTS': '20,36',
                                          'KHCOM_WIZARD_KEYS': '25:1,27:2,29:1,31:1,33:3,35:1,37:3'},
                     files=['hd/music/Title.ogg', 'hd/sprites/gCardWep01Tiles.png', 'hd/subs/ROP.us.txt'], frames=1600)
        print('   ini: hd_cards', got.get('hd_cards'), 'hd_music', got.get('hd_music'), 'hd_subtitles',
              got.get('hd_subtitles'), 'undub', got.get('undub'), 'pictures', sorted(p.name for p in d.glob('wizard_*')))
    if 'newgame' in cases:  # HD mode chosen on the first start, nothing of the HD mod: the toggles keep HD text on
        ini = dict(BASE, graphics_mode=1, graphics_mode_applied=1, hd_text=1, dodge_button=1, stock_button=1)
        keys = '10:3,' + ','.join(f'{20 + i * 10}:1' for i in range(7))
        shots = '5,15,' + ','.join(str(25 + i * 10) for i in range(7))
        # toggles tried on the greyed rows: right on each (2)
        keys += ',' + ','.join(f'{22 + i * 10}:2' for i in range(7))
        got, d = run('newgame', ini, {'KHCOM_KEYS': ROUTE, 'KHCOM_WIZARD_KEYS': keys, 'KHCOM_WIZARD_SHOTS': shots},
                     frames=3000)
        print('   ini: hd_text', got.get('hd_text'), 'hd_music', got.get('hd_music'), 'hd_cards', got.get('hd_cards'),
              'pictures', len(list(d.glob('wizard_*'))))
    if 'confirm_mode' in cases:  # Options > reset to a mode: GBA classic chosen, confirmed twice
        r = it['ITEM_MODE_RESET']
        ini = dict(BASE, game_language=5, hd_text=1)
        got, d = run('confirm_mode', ini, {'KHCOM_OPTIONS_TOGGLE': '700', 'KHCOM_OPTIONS_HOLD': '40', 'KHCOM_OPTIONS_ACTIONS': f'2:{r}:1',
                                           'KHCOM_CONFIRM_KEYS': '4:1,6:2,8:1,10:2,12:1,14:2'}, frames=1000)
        print('   ini: graphics_mode', got.get('graphics_mode'), 'view_mode', got.get('view_mode'), 'hd_text',
              got.get('hd_text'), 'pictures', sorted(p.name for p in d.glob('options_*')))
    if 'confirm_title' in cases:  # Options > back to the title: No at the second question, then yes twice
        r = it['ITEM_TITLE']
        got, d = run('confirm_title', dict(BASE, game_language=5), {'KHCOM_OPTIONS_TOGGLE': '700', 'KHCOM_OPTIONS_HOLD': '60',
                                                  'KHCOM_OPTIONS_ACTIONS': f'2:{r}:1,12:{r}:1',
                                                  'KHCOM_CONFIRM_KEYS': '4:1,6:2,8:2,14:1,16:2,18:1,20:2'},
                     frames=1000)
        print('   pictures', sorted(p.name for p in d.glob('options_*')))
    if 'confirm_states' in cases:  # Cheats > quick states: "No" keeps them off, then yes turns them on, achievements paused
        r = it['ITEM_CHEAT_STATES']
        got, d = run('confirm_states', dict(BASE, game_language=5), {'KHCOM_OPTIONS_TOGGLE': '700', 'KHCOM_OPTIONS_HOLD': '40',
                                                    'KHCOM_OPTIONS_ACTIONS': f'2:{r}:1,10:{r}:1',
                                                    'KHCOM_CONFIRM_KEYS': '4:2,12:1,14:2'}, frames=1000)
        log = (d / 'kh_com.log').read_text(encoding='utf-8', errors='replace')
        print('   ini: cheat_savestates', got.get('cheat_savestates'), 'achievements paused',
              'session paused' in log, 'pictures', sorted(p.name for p in d.glob('options_*')))
    if 'title_last' in cases:  # the main list scrolled to its end
        got, d = run('title_last', dict(BASE, game_language=5), {'KHCOM_OPTIONS_SHOT': '700', 'KHCOM_OPTIONS_CURSOR': str(it['ITEM_TITLE'])},
                     frames=720)
        print('   pictures', sorted(p.name for p in d.glob('*.bmp')))


if __name__ == '__main__':
    main(sys.argv[1:] or ['notice', 'notice_many', 'newgame', 'confirm_mode', 'confirm_title', 'title_last'])
