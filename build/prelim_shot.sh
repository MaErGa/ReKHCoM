#!/bin/sh
# One Preliminar V1 scene as the review desk shows it: 3440x1440 at zoom 6 (x3), frame F of its event, with the
# recipe (after) and without it (before), hidden and silent. Writes build/prelim/<scene>-{after,before}.png.
# usage: sh build/prelim_shot.sh <scene> [frame=300] [list:index]
R="$(cd "$(dirname "$0")/.." && pwd -W)"
SC=$1; F=${2:-300}
EV=${3:-$(cd "$R/port/tools" && python -c "
from test_events import event_lists
ev={int(n.split('_')[1]):(l,i) for l,i,n in event_lists() if n.split('_')[1].isdigit()}
print('%d:%d'%ev[$SC])")}
OUT="$R/build/prelim"; mkdir -p "$OUT"
for mode in after before; do
  D="$OUT/run-$SC-$mode"; rm -rf "$D"; mkdir -p "$D"
  cp "$R/build/pc/khcom.exe" "$R/build/pc/SDL3.dll" "$D/"
  printf 'config_version=11\nview_mode=4\nzoom=6\nvsync=0\nhd_text=0\n' > "$D/kh_com.ini"
  if [ $mode = after ]; then X=KHCOM_PRELIMINAR_V1=1; else X=KHCOM_PRELIMINAR_SKIP=$SC; fi
  (cd "$D" && env SDL_VIDEO_DRIVER=dummy SDL_AUDIO_DRIVER=dummy KHCOM_FAST=1 KHCOM_WINDOW=3440x1440 KHCOM_EVENT=$EV $X \
     KHCOM_KEYS="$(python -c "print(','.join(f'{f}-{f+3}:A' for f in range(30,$F,40)))")" KHCOM_OUTPUT_SHOT=$F \
     KHCOM_QUIT=$((F+4)) timeout 120 ./khcom.exe --rom "$R/roms/B8CP.gba" >/dev/null 2>&1)
  python -c "from PIL import Image; Image.open(r'$D/out_$(printf %05d $F).bmp').save(r'$OUT/$SC-$mode.png')"
done
echo "$OUT/$SC-after.png"
