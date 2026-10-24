"""A worker's checkout, ready in one step and kept between rounds: <work>/wt-<name> on its own branch from origin/main,
the ROMs copied in, the EU game built (incremental after the first time). Prints the PATH line to use.

    python port/tools/worker_setup.py w1            # create or refresh D:/01Claude/ReKHCoM-build/wt-w1, build EU
    python port/tools/worker_setup.py w1 --no-build

The worktree sits next to <work>/toolchain, so configure_pc.py finds it as ../toolchain (no paths across drives).
A worktree with uncommitted changes is only fetched, never reset.
"""
import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).resolve().parent))
from workdir import WORK_DIR  # noqa: E402


def run(*args, cwd=None, check=True):
    print('>', ' '.join(str(a) for a in args), flush=True)
    return subprocess.run([str(a) for a in args], cwd=cwd, check=check)


def git(*args, cwd=REPO, capture=False):
    p = subprocess.run(['git', '-C', str(cwd), *args], capture_output=capture, text=True)
    if p.returncode and not capture:
        raise SystemExit('git ' + ' '.join(args) + ' failed')
    return p.stdout.strip() if capture else p


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('name')
    ap.add_argument('--no-build', action='store_true')
    a = ap.parse_args()
    work = Path(WORK_DIR)
    wt = work / ('wt-' + a.name)
    branch = 'claude/worker-' + a.name
    git('fetch', '-q', 'origin')
    if not wt.exists():
        git('worktree', 'add', '-q', '-B', branch, str(wt), 'origin/main')
    elif git('status', '--porcelain', cwd=wt, capture=True):
        print('uncommitted work in', wt, '- fetched only, not reset')
    else:
        git('checkout', '-q', '-B', branch, 'origin/main', cwd=wt)
    roms = wt / 'roms'
    roms.mkdir(exist_ok=True)
    for name in ('B8CP.gba', 'B8CE.gba', 'B8CJ.gba'):
        src = REPO / 'roms' / name
        if src.exists() and not (roms / name).exists():
            shutil.copy2(src, roms / name)
    tool_bin = work / 'toolchain' / 'bin'
    if not a.no_build:
        env = dict(os.environ, PATH=str(tool_bin) + os.pathsep + os.environ.get('PATH', ''))
        out = wt / 'build' / 'pc-eu'
        if not (out / 'build.ninja').exists():
            subprocess.run([sys.executable, 'port/configure_pc.py', '--version', 'eu', '--out', 'build/pc-eu'],
                           cwd=wt, env=env, check=True)
        subprocess.run([str(tool_bin / 'ninja.exe'), '-f', 'build/pc-eu/build.ninja'], cwd=wt, env=env, check=True)
    print()
    print('worktree:', wt)
    print('branch:  ', branch)
    print('game:    ', wt / 'build' / 'pc-eu' / 'khcom.exe')
    print('PATH:     export PATH="' + tool_bin.as_posix() + ':$PATH"')
    return 0


if __name__ == '__main__':
    sys.exit(main())
