"""The heavy work folder: VM images, Linux sysroots, the HD mod's full import, emulator AVDs, caches. Outside the
repository and never in git (EMPEZAR_AQUI.md). REKHCOM_WORK chooses it; without it, the folder this PC has always
used if it exists, else ReKHCoM-build next to the checkout."""
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
_LEGACY = Path('D:/01Claude/ReKHCoM-build')

if os.environ.get('REKHCOM_WORK'):
    WORK_DIR = Path(os.environ['REKHCOM_WORK'])
elif _LEGACY.is_dir():
    WORK_DIR = _LEGACY
else:
    WORK_DIR = ROOT.parent / 'ReKHCoM-build'
