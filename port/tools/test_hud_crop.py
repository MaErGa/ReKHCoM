"""Whole-window regression: cropped transparent HUD equals full blit, in map and battle."""
import os,shutil,subprocess,json
from pathlib import Path
from PIL import Image,ImageChops
import argparse
ap=argparse.ArgumentParser();ap.add_argument('--build',type=Path,default=Path('build/pc-eu'));args=ap.parse_args()
r=Path(__file__).resolve().parents[2];out=r/'build/hud-crop-proof';out.mkdir(exist_ok=True)
base={k:v for k,v in os.environ.items() if not k.startswith(('KHCOM_','SDL_'))}
for k in ['SDL_XINPUT_ENABLED','SDL_JOYSTICK_HIDAPI','SDL_JOYSTICK_RAWINPUT','SDL_JOYSTICK_DIRECTINPUT','SDL_JOYSTICK_WGI','SDL_JOYSTICK_GAMEINPUT','SDL_JOYSTICK_GAMEINPUT_RAW']:base[k]='0'
base.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',KHCOM_FAST='1')
reports=[]
for scene in ['map','battle']:
 d=out/scene;d.mkdir(exist_ok=True)
 for n in ['khcom.exe','SDL3.dll']:shutil.copy2((r/args.build)/n,d/n)
 shutil.copytree(r/'port/assets/fonts',d/'assets/fonts',dirs_exist_ok=True)
 (d/'kh_com.ini').write_text('config_version=11\nview_mode=4\nzoom=6\nhd_text=1\nvsync=0\nexperimental_frame_skip=0\ndebug_gba_map_logic=2\n')
 e=dict(base,KHCOM_WINDOW='720x480',KHCOM_QUIT='244',KHCOM_SAVESTATE='240:frame240.khst')
 if scene=='map':e.update(KHCOM_MODE='60:room:10901,120:fieldfreeze',KHCOM_KEYS='180-240:UP+RIGHT')
 else:e['KHCOM_BATTLE']='10:4:0'
 subprocess.run([str(d/'khcom.exe'),'--rom',str(r/'roms/B8CP.gba')],cwd=d,env=e,timeout=60,check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
 for w,h in ([(1536,864),(1920,1080),(3840,1080)] if scene=='map' else [(1920,1080)]):
  pair=[]
  for full in [True,False]:
   e=dict(base,KHCOM_WINDOW=f'{w}x{h}',KHCOM_LOADSTATE='frame240.khst',KHCOM_OUTPUT_SHOT='126,132',KHCOM_QUIT='134')
   if full:e['KHCOM_HUD_FULL_BLIT']='1'
   subprocess.run([str(d/'khcom.exe'),'--rom',str(r/'roms/B8CP.gba')],cwd=d,env=e,timeout=60,check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
   assert '-> 1' in (d/'kh_com.log').read_text(errors='replace')
   shots=[]
   for f in [126,132]:
    im=Image.open(d/f'out_{f:05d}.bmp').convert('RGB');im.save(d/f'{w}-{f}-{full}.png');shots.append(im)
   pair.append(shots)
  for f,a,b in zip([126,132],*pair):
   diff=ImageChops.difference(a,b).getbbox();reports.append(dict(scene=scene,window=[w,h],frame=f,diff=diff));assert diff is None,reports[-1]
  print(scene,w,'pixel identical',flush=True)
(out/'report.json').write_text(json.dumps(reports,indent=2))

