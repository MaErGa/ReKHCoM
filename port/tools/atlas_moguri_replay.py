import os,sys,subprocess,shutil,json,time
from pathlib import Path
from PIL import Image

root=Path(__file__).resolve().parents[2];os.chdir(root);out=root/'build/atlas-moguri-after';out.mkdir(exist_ok=True);report=[]
hd=(root.parent/'khcom-claude-int/build/hdfull/hd/sprites').resolve();save=root.parent/'respaldos/saves/jugar/ReKHCoM-v1.9.15-completo/kh_com.sav'
jobs=[('comprar',dict(KHCOM_KEYS=','.join(f'{f}-{f+3}:A' for f in range(200,901,50)),KHCOM_MODE='1000:mode_ms_shop'),1500),('canjear',dict(KHCOM_KEYS=','.join(f'{f}-{f+3}:A' for f in range(200,901,50)),KHCOM_MODE='1000:mode_ms_charge'),1500)]
for name,extra,check in jobs:
 d=out/name;d.mkdir(exist_ok=True)
 for f in ('khcom.exe','SDL3.dll'):shutil.copy2(root/'build/pc-atlas'/f,d/f)
 if not(d/'assets/fonts').exists():shutil.copytree(root/'port/assets/fonts',d/'assets/fonts')
 if name!='title':shutil.copy2(save,d/'kh_com.sav')
 (d/'kh_com.ini').write_text('config_version=11\nview_mode=4\nzoom=6\nhd_text=1\nhd_cards=1\nhd_title=1\nhd_font=0\nvsync=0\n')
 e={k:v for k,v in os.environ.items() if not k.startswith(('KHCOM_','SDL_'))};e.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT='0xffff/0xffff',KHCOM_FAST='1',KHCOM_PPU_THREADS='1',KHCOM_BACKGROUND_TEST='1',KHCOM_WINDOW='1920x1080',KHCOM_HD_SPRITES=str(hd),KHCOM_CONFIG='hd_text=1;hd_cards=1;hd_title=1;hd_font=0',KHCOM_OUTPUT_SHOT=f'{check-80},{check}',KHCOM_HDDRAWLOG=str(check),KHCOM_QUIT=str(check+4));e.update(extra)
 with(d/'runtime.log').open('w') as log:subprocess.run([str(d/'khcom.exe'),'--language','es','--rom',str(root/'roms/B8CP.gba')],cwd=d,env=e,stdout=log,stderr=log,timeout=150,check=True,creationflags=subprocess.BELOW_NORMAL_PRIORITY_CLASS)
 board=Image.new('RGB',(1920,2160))
 for i,f in enumerate((check-80,check)):
  im=Image.open(d/f'out_{f:05d}.bmp').convert('RGB');im.save(d/f'{f}.png');board.paste(im,(0,i*1080))
 board.save(d/'review.png');thumb=im.copy();thumb.thumbnail((960,540));thumb.save(d/'thumb.png');report.append(dict(case=name,frames=[check-80,check],review=str(d/'review.png')));(out/'report.json').write_text(json.dumps(report,indent=2));print(name,'capturado',flush=True)
