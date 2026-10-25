import sys,json,re,hashlib
from pathlib import Path
sys.path.insert(0,str(Path('port/tools/art_review').resolve()))
from revise_darkside_boundary import frame,extension
from darkside_revise import pack_inc,draw
from PIL import Image,ImageDraw,ImageFont
root=Path.cwd(); desk=root.parent/'khcom-codex/build/art-review'; source=root.parent/'khcom-claude-int'
out=root/'build/atlas-jefes/darkside06';out.mkdir(parents=True,exist_ok=True)
original,palette=frame(source,13)
inc=(root/'port/include/port/darkside_body_punch.inc').read_text()
r6=extension((desk/'proposals/darkside-r6/darkside_body_punch_r6.inc').read_text())
old={(x,y+11):c for (x,y),c in r6.items() if x>=256 and (x,y+11) not in original}
for y in range(min(y for x,y in old),max(y for x,y in old)):
 if (255,y) in original and (256,y) not in old and (257,y) in old:old[256,y]=original[255,y]
pts=list(original)+list(old); bx=max(0,min(x for x,y in pts)-8);by=max(0,min(y for x,y in pts)-8)
payload=json.loads((desk/'envios/20261024-162822-ea1330/comentarios.json').read_text(encoding='utf8'))
a=next(a for a in payload['assets'] if a['id']=='4d6a039e51ad4b8f8f52cb1b1d070ba0')
print('box',bx,by,'doodles',str(a['doodles'])[:500])
new=dict(old)
# Doodles address the natural left pane at 2x. Native ROM pixels always win.
marked=Image.open(desk/'envios/20261024-162822-ea1330/4d6a039e51ad4b8f8f52cb1b1d070ba0-garabatos.png').convert('RGB')
erased=filled=0
for y in range(256):
 for x in range(256,360):
  sx=24+(x-bx)*2;sy=84+(y-by)*2
  if not (0<=sx<marked.width-1 and 0<=sy<marked.height-1):continue
  colors=[marked.getpixel((sx+dx,sy+dy)) for dx in (0,1) for dy in (0,1)]
  if any(b>100 and g>100 and r<40 for r,g,b in colors):
   if (x,y) in new:del new[x,y];erased+=1
  elif any(g>100 and g>r*1.35 and g>b*1.2 for r,g,b in colors):
   # Sample the nearest existing plane, using native shoulder at equal height first.
   near=[(abs(xx-x)+abs(yy-y)*2,c) for (xx,yy),c in {**old,**original}.items() if abs(xx-x)<=12 and abs(yy-y)<=5]
   if near:new[x,y]=min(near)[1];filled+=1
new={p:c for p,c in new.items() if p not in original}
# Close only 1px holes at the fitted seam, never the deliberate open armpit.
for y in range(256):
 if (255,y) in original and (257,y) in new and (256,y) not in new and y<100:new[256,y]=original[255,y]
modified=pack_inc(inc,'punch',new,'DARKSIDE_PUNCH_TOP')
(out/'darkside_body_punch_r8.inc').write_text(modified,encoding='utf8')
assert extension(modified)==new
checks=[]
for slot in [13,*range(21,29)]:
 base,_=frame(source,slot);merged={**new,**base}
 assert all(merged[p]==c for p,c in base.items())
 checks.append({'frame':slot,'native_pixels_changed':0})
info={'response_to':[{'submission':payload['submission_id'],'image_id':a['id'],'cloud_id':n['id']} for n in a['annotations']]+[{'submission':payload['submission_id'],'image_id':a['id'],'cloud_id':d['id']} for d in a['doodles']], 'change':'Costura local: quitar el añadido marcado celeste y rellenar las dos uniones verdes con índices del mismo hombro. Centro ROM intacto.', 'technique':'4bpp nativo, máscaras del garabato original, sin espejo; paleta y resolución ROM conservadas.', 'checks':checks,'removed_added_pixels':erased,'green_fill_pixels':filled,'limitations':['Propuesta offline; pendiente de captura runtime por turno de RAM. No aprobada.'],'subsequent_human_verdict':None}
(out/'revision.json').write_text(json.dumps(info,ensure_ascii=False,indent=2),encoding='utf8')
draw(out/'darkside06-r8.png','06 DARKSIDE / costura nueva / propuesta offline',['Celeste: añadido borrado. Verde: uniones rellenas con la misma paleta.','Centro ROM intacto en los 9 frames de puñetazo. Pendiente captura runtime.'],original,new,palette)
print(json.dumps({'removed':erased,'filled':filled,'output':str(out)}))
