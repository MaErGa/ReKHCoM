"""Replay the left filter transition with isolated input; compare a reference PC build.
Usage: python port/tools/test_menu_tab_sync.py --reference <build-dir> --build build/pc-eu
"""
import argparse,json,os,shutil,subprocess
from pathlib import Path
from PIL import Image,ImageChops
ROOT=Path(__file__).resolve().parents[2]
FRAMES=list(range(620,641))+list(range(660,681))
def capture(build,out):
 out.mkdir(parents=True,exist_ok=True)
 for name in ('khcom.exe','SDL3.dll'):shutil.copy2(build/name,out/name)
 (out/'kh_com.ini').write_text('config_version=11\nview_mode=4\nzoom=6\nhd_text=0\nvsync=0\n')
 env={k:v for k,v in os.environ.items() if not k.startswith(('KHCOM_','SDL_'))}
 for key in ('SDL_XINPUT_ENABLED','SDL_JOYSTICK_HIDAPI','SDL_JOYSTICK_RAWINPUT','SDL_JOYSTICK_DIRECTINPUT','SDL_JOYSTICK_WGI','SDL_JOYSTICK_GAMEINPUT','SDL_JOYSTICK_GAMEINPUT_RAW'):env[key]='0'
 env.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',KHCOM_FAST='1',KHCOM_WINDOW='1536x864',KHCOM_MODE='60:room:10901,120:fieldfreeze,180:Mode_Deck',KHCOM_KEYS='420-422:LEFT,500-502:UP,530-532:UP,560-562:UP,590-592:UP,620-622:RIGHT,660-662:LEFT',KHCOM_OUTPUT_SHOT=','.join(map(str,FRAMES)),KHCOM_QUIT='682')
 subprocess.run([str(out/'khcom.exe'),'--language','es','--rom',str(ROOT/'roms/B8CP.gba')],cwd=out,env=env,timeout=120,check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
 return {f:Image.open(out/f'out_{f:05d}.bmp').convert('RGB') for f in FRAMES}
def compare(before,after):
 native=[];changed=[]
 for f in FRAMES:
  diff=ImageChops.difference(before[f],after[f])
  if diff.crop((120,0,1416,864)).getbbox():native.append(f)
  if diff.getbbox():changed.append(f)
 def seam(im):return sum(im.getpixel((116,y))!=im.getpixel((123,y)) for y in range(90,168))
 row=dict(native_changed=native,changed_frames=changed,before_seam=seam(before[661]),after_seam=seam(after[661]))
 assert not native,row
 assert row['before_seam']>0 and row['after_seam']==0,row
 assert ImageChops.difference(before[680],after[680]).getbbox() is None,row
 return row
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--reference',type=Path,required=True);ap.add_argument('--build',type=Path,default=Path('build/pc-eu'));a=ap.parse_args()
 out=ROOT/'build/menu-tab-sync';result=compare(capture(a.reference.resolve(),out/'before'),capture(a.build.resolve(),out/'after'))
 (out/'report.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
