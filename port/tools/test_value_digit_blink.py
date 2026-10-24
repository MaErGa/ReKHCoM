"""Regression: selected Add Cards counts stay visible throughout the selector pulse (EU ROM)."""
import os,shutil,subprocess
from pathlib import Path
from PIL import Image,ImageDraw
import argparse
ap=argparse.ArgumentParser();ap.add_argument('--build',type=Path,default=Path('build/pc-eu'));args=ap.parse_args()
r=Path(__file__).resolve().parents[2];d=r/'build/value-digit-blink';d.mkdir(exist_ok=True)
for n in ['khcom.exe','SDL3.dll']:shutil.copy2((r/args.build)/n,d/n)
(d/'kh_com.ini').write_text('config_version=11\nview_mode=4\nzoom=6\nhd_text=1\nvsync=0\n')
frames=list(range(480,541,3))
e=dict(os.environ,SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',KHCOM_FAST='1',KHCOM_HD_TEXT_TRACE=','.join(map(str,frames)),KHCOM_WINDOW='1536x864',KHCOM_MODE='60:room:10901,120:fieldfreeze,150:cheat:3,180:Mode_Deck',KHCOM_KEYS='300-302:R,420-422:A,460-462:DOWN',KHCOM_OUTPUT_SHOT=','.join(map(str,frames)),KHCOM_SHOTS=','.join(map(str,frames)),KHCOM_QUIT='542')

for hint in ['SDL_XINPUT_ENABLED','SDL_JOYSTICK_HIDAPI','SDL_JOYSTICK_RAWINPUT','SDL_JOYSTICK_DIRECTINPUT','SDL_JOYSTICK_WGI','SDL_JOYSTICK_GAMEINPUT','SDL_JOYSTICK_GAMEINPUT_RAW']:e[hint]='0'
subprocess.run([str(d/'khcom.exe'),'--language','es','--rom',str(r/'roms/B8CP.gba')],cwd=d,env=e,check=True,timeout=120,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
import re,json
counts=[int(x) for x in re.findall(r'hd-text: frame=\d+ layer=0 flowing=\d+ subtitles=\d+ small=(\d+)',(d/'kh_com.log').read_text())]
assert len(counts)==len(frames) and len(set(counts))==1 and counts[0]>=55,counts
(d/'report.json').write_text(json.dumps(dict(frames=frames,small=counts),indent=2));print('stable numeric glyphs:',counts[0])
