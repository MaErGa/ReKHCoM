"""Every-frame regression: HD menu digits must not paint through native occluders."""
import argparse,json,os,re,shutil,subprocess
from pathlib import Path
from PIL import Image,ImageChops
p=argparse.ArgumentParser();p.add_argument('--build',type=Path,default=Path('build/pc-eu'));a=p.parse_args()
root=Path(__file__).resolve().parents[2];out=root/'build/value-digit-occlusion';out.mkdir(exist_ok=True)
frames=list(range(480,541));rows={};reports=[]
for hd in (0,1):
 d=out/str(hd);d.mkdir(exist_ok=True)
 for name in ('khcom.exe','SDL3.dll'):shutil.copy2(root/a.build/name,d/name)
 (d/'kh_com.ini').write_text(f'config_version=11\nview_mode=4\nzoom=6\nhd_text={hd}\nvsync=0\n')
 env=dict(os.environ,SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT='0xffff/0xffff',SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',KHCOM_FAST='1',KHCOM_PPU_THREADS='1',KHCOM_BACKGROUND_TEST='1',KHCOM_HD_TEXT_COVERAGE='480',KHCOM_WINDOW='1536x864',KHCOM_MODE='60:room:10901,120:fieldfreeze,150:cheat:3,180:Mode_Deck',KHCOM_KEYS='300-302:R,420-422:A,460-462:DOWN',KHCOM_OUTPUT_SHOT=','.join(map(str,frames)),KHCOM_QUIT='542')
 r=subprocess.run([str(d/'khcom.exe'),'--language','es','--rom',str(root/'roms/B8CP.gba')],cwd=d,env=env,capture_output=True,timeout=240);assert r.returncode==0,r.returncode
 log=(d/'kh_com.log').read_text(errors='replace');assert 'CRASH' not in log and 'soft reset' not in log.lower()
 if hd:
  for f,layer,x,y,code,ink,bits in re.findall(r'text-cover: frame=(\d+) layer=(\d+) x=(-?\d+) y=(-?\d+) code=(\d+) ink=(\d+) rows=([0-9a-f]{64})',log):
   if int(layer)==0:rows.setdefault(int(f),[]).append((int(x),int(y),int(code),int(ink),[int(bits[i:i+4],16) for i in range(0,64,4)]))
 for f in frames:
  im=Image.open(d/f'out_{f:05d}.bmp').convert('RGB');im.save(d/f'{f}.png')
 if hd:
  for f in frames:
   before=Image.open(out/'0'/f'{f}.png');after=Image.open(d/f'{f}.png');assert before.size==after.size==(1536,864)
   # The 512x288 world canvas is rendered at3x. Compare actual images,
   # not glyph counts: no HD ink may appear in the game's covered cells.
   diff=ImageChops.difference(before,after);checked=partial=0
   assert rows.get(f),f'no native coverage for frame{f}'
   for x,y,code,ink,mask in rows[f]:
    if ink and any(m not in (0,65535) for m in mask):partial+=1
    for iy,bits in enumerate(mask):
     for ix in range(16):
      if bits&(1<<ix):continue
      px=(x+ix-4)*3;py=(y+iy-4)*3
      if px<0 or py<0 or px+3>1536 or py+3>864:continue
      assert diff.crop((px,py,px+3,py+3)).getbbox() is None,(f,x,y,code,ix,iy)
      checked+=1
   reports.append(dict(frame=f,covered_cells=checked,partial_glyphs=partial))
assert sum(r['covered_cells'] for r in reports)>0,'fixture has no covered cells'
assert sum(r['partial_glyphs'] for r in reports)>0,'fixture never partially covers a number'
(out/'report.json').write_text(json.dumps(reports,indent=2));print(f'{len(frames)} frames: covered native cells preserved; partial glyphs checked')
