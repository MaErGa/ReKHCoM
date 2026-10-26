"""Run a Windows game copied without assets; both core fonts must still load."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from test_events import event_lists

ROOT = Path(__file__).resolve().parents[2]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build', type=Path, default=ROOT / 'build/pc')
    p.add_argument('--rom', type=Path, default=ROOT / 'roms/B8CP.gba')
    args = p.parse_args()
    work = ROOT / 'build'
    work.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='core-fonts-', dir=work) as tmp:
        case = Path(tmp).resolve()
        assert case.parent == work.resolve()
        for name in ('khcom.exe', 'SDL3.dll'):
            shutil.copy2(args.build / name, case / name)
        (case / 'kh_com.ini').write_text(
            'config_version=11\nview_mode=4\nzoom=6\nhd_text=1\nhd_font=0\nvsync=0\n')
        event = next((l, i) for l, i, name in event_lists() if name.split('_')[1] == '107')
        env = {k: v for k, v in os.environ.items() if not k.startswith(('KHCOM_', 'SDL_'))}
        env.update(SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy',
                   SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT='0xffff/0xffff',
                   KHCOM_FAST='1', KHCOM_PPU_THREADS='1', KHCOM_BACKGROUND_TEST='1',
                   KHCOM_EVENT='%d:%d' % event, KHCOM_WINDOW='1920x1080', KHCOM_QUIT='304',
                   KHCOM_KEYS=','.join(f'{f}-{f+3}:A' for f in range(30, 301, 40)))
        result = subprocess.run([str(case / 'khcom.exe'), '--data-dir', str(case),
                                 '--rom', str(args.rom.resolve())], cwd=case, env=env,
                                capture_output=True, timeout=100)
        assert result.returncode == 0, result.returncode
        log = '\n'.join(f.read_text(encoding='utf-8', errors='replace') for f in case.glob('*.log'))
        for face in ('KHMenu', 'Comic Hearts'):
            assert f'font {face} (embedded core resource)' in log, log[-2000:]
        assert 'using fallback' not in log
        assert not (case / 'assets/fonts').exists()
    print('PASS: loose executable retains both core fonts without external assets')


if __name__ == '__main__':
    main()
