"""Gallery transition matrix, with isolated reusable workers and explicit unavailable/N/A results.

Codec fixtures are synthetic local MP4s, not story footage. Saves/ROMs are copied and never modified at source.
Writes one bounded log per case, fingerprints, exit codes, and an append-only results.jsonl for resuming.
"""
import argparse
import concurrent.futures
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import threading
import time

ROOT = Path(__file__).resolve().parents[2]
MOVIE_FILES = {513: ('ROP', 0), 514: (None, 0), 515: ('zz_event_123', 170),
               516: ('zz_event_151', 222), 517: ('zz_event_631', 0)}


def key(case):
    return '-'.join(str(case[k]) for k in ('mode', 'context', 'actor', 'tab', 'event', 'resource', 'scenes', 'movies', 'exit', 'recom', 'window'))


def cases(which):
    for mode, context, actor, tab, resource, scenes, route, kind in itertools.product(
            (1, 2, 3), ('title', 'game'), ('sora', 'riku'), (0, 1, 2), ('absent', 'valid', 'error'),
            (0, 1), ('finish', 'skip', 'cancel'), ('mapped', 'native')):
        event = (10 if actor == 'sora' else 155) if kind == 'mapped' else (2 if actor == 'sora' else 150)
        yield dict(mode=mode, context=context, actor=actor, tab=tab, event=event, resource=resource,
                   scenes=scenes, movies=1, exit=route, recom=0, window='240x160', kind=kind)
    if which == 'full':
        for mode, context, tab, resource, scenes, movies, route, event in itertools.product(
                (1, 2, 3), ('title', 'game'), (0, 1, 2), ('absent', 'valid', 'error'),
                (0, 1), (0, 1), ('finish', 'skip', 'cancel'), range(513, 518)):
            yield dict(mode=mode, context=context, actor='riku' if event == 517 else 'sora', tab=tab,
                       event=event, resource=resource, scenes=scenes, movies=movies, exit=route,
                       recom=0, window='240x160', kind='cinematic')
        for mode, context, actor, recom, scenes, route in itertools.product(
                (1, 2, 3), ('title', 'game'), ('sora', 'riku'), (0, 1), (0, 1), ('finish', 'skip', 'cancel')):
            yield dict(mode=mode, context=context, actor=actor, tab=2, event=0 if actor == 'sora' else 193,
                       resource='valid', scenes=scenes, movies=1, exit=route, recom=recom,
                       window='240x160', kind='extras')


def movie_for(event):
    if event in MOVIE_FILES:
        return MOVIE_FILES[event]
    rows = dict((int(n), movie) for n, movie in re.findall(r'\{ EVENT_(\d+)[A-Z0-9_]*, "([^"]+)"',
                (ROOT / 'port/src/port_hdscenes.inc').read_text(encoding='utf-8')))
    return rows.get(event), 0


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(pool, exe, rom):
    pool.mkdir(parents=True, exist_ok=True)
    for name, source in [('khcom.exe', exe), ('SDL3.dll', exe.parent / 'SDL3.dll'), ('baserom.gba', rom)]:
        target = pool / name
        if not target.exists() or target.stat().st_mtime != source.stat().st_mtime:
            shutil.copy2(source, target)
    shutil.copytree(exe.parent / 'assets', pool / 'assets', dirs_exist_ok=True)


def run_case(case, pool, work, fixtures, save, repeats=2, screenshots=False):
    ident = key(case)
    movie, offset = movie_for(case['event'])
    if case['tab'] == 1 and not movie:
        return dict(id=ident, case=case, status='N/A', reason='The HD-only list excludes events/cinematics without an HD mapping.')
    # Remove only this worker's generated cache/media; linked source fixtures are unlinked before any replacement.
    for path in list(pool.glob('scene-preview-*.rgb')) + list(pool.glob('gallery_*.bmp')) + list((pool / 'hd').rglob('*.mp4')):
        path.unlink()
    for path in pool.glob('achievements_*.dat'):
        path.unlink()
    if save:
        shutil.copy2(save, pool / 'kh_com.sav')
    else:
        (pool / 'kh_com.sav').write_bytes(b'\xff' * 65536)
    before_save = sha(pool / 'kh_com.sav')
    (pool / 'scenes_seen.dat').write_bytes(b'\xff' * 128)
    provider_movie = movie or ('zz_event_012' if case['actor'] == 'sora' else 'zz_event_503')
    if case['resource'] != 'absent':
        target = pool / 'hd/scenes' / (provider_movie + '.mp4')
        target.parent.mkdir(parents=True, exist_ok=True)
        if case['resource'] == 'error':
            target.write_bytes(b'Invalid local codec fixture')
        else:
            os.link(fixtures[offset], target)
        if case['kind'] == 'extras':
            extra = pool / 'hd/scenes' / ('zz_event_001.mp4' if case['actor'] == 'sora' else 'zz_event_614.mp4')
            os.link(fixtures[0], extra)
    (pool / 'kh_com.ini').write_text(
        f"config_version=11\ngraphics_mode={case['mode']}\ngraphics_mode_applied={case['mode']}\n"
        f"hd_title={int(case['mode'] == 1)}\nhd_text={int(case['mode'] != 3)}\nhd_hud=0\nhd_cards=0\n"
        f"hd_scenes={case['scenes']}\nhd_movies={case['movies']}\nhd_music=0\nassets_seen=255\n"
        f"ps2_mode={case['recom']}\nps2_difficulty=2\nfullscreen=0\nview_mode={0 if case['mode'] == 3 else 4}\n"
        "game_language=5\nzoom=8\nvsync=0\nsave_bank=1\n", encoding='utf-8')
    events = [f'{30 + 40 * i}:120' for i in range(repeats)]
    close_tick = 30 + 40 * repeats
    events.append(f'{close_tick}:-1')
    env = {k: v for k, v in os.environ.items() if not k.startswith('KHCOM_')}
    env.update(SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1', KHCOM_BACKGROUND_TEST='1',
               KHCOM_WINDOW=case['window'], KHCOM_MODE=f"1:{'roomriku' if case['actor'] == 'riku' else 'room'}:20000" if case['context'] == 'game' else '1:mode_title',
               KHCOM_OPTIONS_AT='120', KHCOM_SCENES_SHOT=str(case['tab'] + 1), KHCOM_GALLERY_EVENT=str(case['event'] - 2 if 129 <= case['event'] < 512 else case['event']),
               KHCOM_GALLERY_KEYS=','.join(events), KHCOM_GALLERY_AUDIT='1', KHCOM_QUIT='8000',
               KHCOM_GALLERY_QUIT_AFTER=str(close_tick), KHCOM_ACHIEVEMENTS_LIVE='1')
    if case['recom']:
        env['KHCOM_RECOM'] = '1'
    if screenshots:
        env['KHCOM_GALLERY_SHOTS'] = '25,' + str(close_tick - 5)
        env['KHCOM_GALLERY_PREVIEWTEST'] = '1'
    hd = movie and case['tab'] in (1, 2) and case['resource'] == 'valid'
    if hd and case['exit'] in ('skip', 'cancel'):
        env['KHCOM_HDMOVIE_PAUSETEST'] = '0'
        env['KHCOM_HDMOVIE_PAUSE_ACTION'] = case['exit']
    if not hd:
        env['KHCOM_GALLERY_AUTO_KEYS'] = case['exit']
        if case['exit'] != 'finish':
            env['KHCOM_GALLERY_NATIVE_PAUSE'] = 'cancel' if case['exit'] == 'cancel' else 'skip'
    log_path = work / 'logs' / (ident + '.log')
    log_path.parent.mkdir(exist_ok=True)
    started = time.monotonic()
    try:
        with log_path.open('w', encoding='utf-8') as log:
            run = subprocess.run([str(pool / 'khcom.exe')], cwd=pool, env=env, stdout=log,
                                 stderr=subprocess.STDOUT, timeout=180)
        text = log_path.read_text(encoding='utf-8', errors='replace')
        if run.returncode:
            raise AssertionError(f'exit={run.returncode}')
        if re.search(r'bios:|CRASH|FATAL', text):
            raise AssertionError('Invalid memory access/crash/fatal in log')
        audits = re.findall(r'scene gallery audit: game=(\d) map=(\d) config=(\d) files=(\d) recom=(\d) rank=(\d)', text)
        if any(a != ('1',) * 6 for a in audits):
            raise AssertionError('Replay modified game/map/config/achievement/scene progress or Re:CoM state: ' + str(audits))
        unavailable = case['tab'] == 1 and case['resource'] == 'absent'
        if not unavailable and len(audits) < repeats:
            raise AssertionError(f'Expected {repeats} completed audits; got {len(audits)}')
        if sha(pool / 'kh_com.sav') != before_save:
            raise AssertionError('SRAM file changed')
        if case['resource'] == 'valid' and hd and text.count('scene gallery: HD ') < repeats:
            raise AssertionError('Expected imported HD provider was not played')
        if not unavailable and not hd and case['resource'] != 'error' and text.count('restored original state; returning to the list') < repeats:
            raise AssertionError('Native scene never returned to its list')
        if f'tick={close_tick} key=-1' not in text:
            raise AssertionError('Gallery did not remain navigable through the final close')
        result = dict(id=ident, case=case, status='PASS', seconds=round(time.monotonic() - started, 3),
                      audits=len(audits), sram_sha256=before_save, log=str(log_path),
                      availability='blocked: HD not imported' if unavailable else 'played')
        if screenshots:
            dest = work / 'shots' / ident
            dest.mkdir(parents=True, exist_ok=True)
            for path in pool.glob('gallery_*.bmp'):
                shutil.copy2(path, dest / path.name)
            result['shots'] = str(dest)
        return result
    except Exception as error:
        return dict(id=ident, case=case, status='FAIL', error=str(error), log=str(log_path),
                    seconds=round(time.monotonic() - started, 3))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', required=True, type=Path)
    p.add_argument('--rom', required=True, type=Path)
    p.add_argument('--work', required=True, type=Path)
    p.add_argument('--save', type=Path)
    p.add_argument('--suite', choices=('pilot', 'core', 'full'), default='pilot')
    p.add_argument('--workers', type=int, default=4)
    p.add_argument('--resume', action='store_true')
    a = p.parse_args()
    a.exe, a.rom, a.work = a.exe.resolve(), a.rom.resolve(), a.work.resolve()
    a.work.mkdir(parents=True, exist_ok=True)
    fixtures = {}
    for offset in (0, 170, 222):
        path = a.work / f'codec-offset-{offset}.mp4'
        if not path.exists():
            subprocess.run(['ffmpeg', '-v', 'error', '-nostdin', '-f', 'lavfi', '-i', 'testsrc2=s=160x90:r=5',
                            '-t', str(offset + 1.5), '-an', '-c:v', 'libx264', '-preset', 'ultrafast',
                            '-crf', '35', '-y', str(path)], check=True)
        fixtures[offset] = path
    jobs = list(cases('full' if a.suite == 'full' else 'core'))
    if a.suite == 'pilot':
        wanted = []
        for mode in (1, 2, 3):
            for actor in ('sora', 'riku'):
                for kind, tab, route, resource in [('mapped', 0, 'finish', 'valid'), ('native', 2, 'cancel', 'valid'),
                                                  ('mapped', 2, 'skip', 'valid'), ('mapped', 1, 'finish', 'error')]:
                    wanted.append(next(c for c in jobs if (c['mode'], c['actor'], c['kind'], c['tab'], c['exit'], c['resource'], c['context'], c['scenes']) ==
                                       (mode, actor, kind, tab, route, resource, 'game', 1)))
        jobs = wanted
    results = a.work / 'results.jsonl'
    done = set()
    if a.resume and results.exists():
        done = {r['id'] for line in results.read_text(encoding='utf-8').splitlines() if (r := json.loads(line))['status'] in ('PASS', 'N/A')}
    jobs = [c for c in jobs if key(c) not in done]
    manifest = dict(exe_sha256=sha(a.exe), rom_sha256=sha(a.rom), suite=a.suite, cases=len(jobs),
                    fixtures={str(o): sha(path) for o, path in fixtures.items()})
    (a.work / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    pools = [a.work / f'worker-{i}' for i in range(a.workers)]
    for pool in pools:
        prepare(pool, a.exe, a.rom)
    lock = threading.Lock()
    counts = dict(PASS=0, FAIL=0, **{'N/A': 0})
    pending = iter(jobs)
    def worker(pool):
        while True:
            with lock:
                case = next(pending, None)
            if case is None:
                return
            result = run_case(case, pool, a.work, fixtures, a.save, screenshots=a.suite == 'pilot')
            with lock:
                with results.open('a', encoding='utf-8') as f:
                    f.write(json.dumps(result, ensure_ascii=False) + '\n')
                counts[result['status']] += 1
                if result['status'] == 'FAIL' or sum(counts.values()) % 25 == 0:
                    print(counts, result['id'], result.get('error', ''), flush=True)
    with concurrent.futures.ThreadPoolExecutor(a.workers) as executor:
        list(executor.map(worker, pools))
    print(json.dumps(counts), flush=True)
    return int(counts['FAIL'] != 0)


if __name__ == '__main__':
    raise SystemExit(main())
